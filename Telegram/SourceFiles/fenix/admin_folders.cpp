/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/admin_folders.h"

#include "apiwrap.h"
#include "core/application.h"
#include "data/data_changes.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_chat_filters.h"
#include "data/data_folder.h"
#include "data/data_premium_limits.h"
#include "data/data_session.h"
#include "dialogs/dialogs_indexed_list.h"
#include "dialogs/dialogs_main_list.h"
#include "dialogs/dialogs_row.h"
#include "history/history.h"
#include "lang/lang_instance.h"
#include "lang/lang_keys.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "storage/storage_account.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSaveFile>

namespace Fenix {
namespace {

using Kind = AdminFolders::Kind;
using Flag = Data::ChatFilter::Flag;

constexpr auto kKindCount = AdminFolders::kKindCount;

// Telegram refuses a longer folder title (Android's MAX_FOLDER_NAME).
constexpr auto kMaxTitle = 12;

// At most one look at the rights per this interval.
constexpr auto kSyncInterval = 10 * crl::time(1000);

// Lets a burst of updates (a page of chats arriving) settle into one look.
constexpr auto kSyncDelay = crl::time(1500);

// An operation whose reply never came stops blocking the others after this.
constexpr auto kBusyStale = 60 * crl::time(1000);

constexpr auto kFormat = 1;

// On every folder of ours: a title has to carry it AND be one of the texts
// below. One UTF-16 unit of the twelve.
constexpr auto kMarker = char16_t(0x2605);

using TitleTexts = std::array<QString, 4>; // en, uz, ru, ar.

// Exactly Android's titles 393-396 (LanguageCode, LanguageCodeAr): a folder
// made there, in any of its languages, has to be recognized here.
[[nodiscard]] const std::array<TitleTexts, kKindCount> &Texts() {
	static const auto result = std::array<TitleTexts, kKindCount>{ {
		{ u"My groups"_q, u"Guruhlarim"_q, u"Мои группы"_q, u"مجموعاتي"_q },
		{
			u"Admin group"_q,
			u"Admin guruh"_q,
			u"Админ груп"_q,
			u"مجموعة مشرف"_q,
		},
		{ u"My channels"_q, u"Kanallarim"_q, u"Мои каналы"_q, u"قنواتي"_q },
		{
			u"Adm channel"_q,
			u"Admin kanal"_q,
			u"Админ канал"_q,
			u"قناة مشرف"_q,
		},
	} };
	return result;
}

[[nodiscard]] int LanguageIndex() {
	const auto code = Lang::GetInstance().id().split('-').front();
	return (code == u"uz"_q)
		? 1
		: (code == u"ru"_q)
		? 2
		: (code == u"ar"_q)
		? 3
		: 0;
}

[[nodiscard]] QString MarkedTitle(const QString &text) {
	return (QString(QChar(kMarker)) + text).left(kMaxTitle);
}

[[nodiscard]] QString TitleFor(Kind kind) {
	return MarkedTitle(Texts()[int(kind)][LanguageIndex()]);
}

[[nodiscard]] bool IsTitleOf(Kind kind, const QString &title) {
	for (const auto &text : Texts()[int(kind)]) {
		// A blank text would match a folder named just the marker.
		if (!text.trimmed().isEmpty() && (title == MarkedTitle(text))) {
			return true;
		}
	}
	return false;
}

[[nodiscard]] bool IsOurTitle(const QString &title) {
	for (auto i = 0; i != kKindCount; ++i) {
		if (IsTitleOf(Kind(i), title)) {
			return true;
		}
	}
	return false;
}

// A folder shared by a link is someone's chat list: never ours to adopt,
// rewrite or delete, even under one of our titles.
[[nodiscard]] bool Usable(const Data::ChatFilter &filter) {
	return filter.id() && !filter.chatlist();
}

[[nodiscard]] QString StatePath(not_null<Main::Session*> session) {
	// Next to the account's cache database, like the saved history.
	return QFileInfo(session->local().cachePath()).absolutePath()
		+ u"/novagram_admin_folders.json"_q;
}

using Registry = base::flat_map<
	not_null<Main::Session*>,
	std::unique_ptr<AdminFolders>>;

[[nodiscard]] Registry &Stored() {
	static auto result = Registry();
	return result;
}

} // namespace

struct AdminFolders::Classified {
	// False when the peer is not loaded well enough to tell.
	bool known = false;
	std::optional<Kind> kind;
};

struct AdminFolders::Running {
	std::vector<Step> steps;
	Fn<void()> finished;
	int generation = 0;
};

AdminFolders::AdminFolders(not_null<Main::Session*> session)
: _session(session)
, _path(StatePath(session))
, _syncTimer([=] { sync(); }) {
	load();

	using PeerFlag = Data::PeerUpdate::Flag;
	_session->changes().peerUpdates(
		PeerFlag::Rights | PeerFlag::ChannelAmIn | PeerFlag::Migration
	) | rpl::filter([](const Data::PeerUpdate &update) {
		return update.peer->isChat() || update.peer->isChannel();
	}) | rpl::on_next([=] {
		scheduleSync();
	}, _lifetime);

	const auto data = &_session->data();
	rpl::merge(
		data->chatsListChanges() | rpl::to_empty,
		data->chatsListLoadedEvents() | rpl::to_empty,
		data->chatsFilters().changed(),
		data->chatListEntryRefreshes(
		) | rpl::filter([](const Data::Session::ChatListEntryRefresh &e) {
			return e.existenceChanged && !e.filterId;
		}) | rpl::to_empty
	) | rpl::on_next([=] {
		scheduleSync();
	}, _lifetime);

	scheduleSync();
}

AdminFolders::~AdminFolders() = default;

bool AdminFolders::enabled() const {
	return _enabled.current();
}

rpl::producer<bool> AdminFolders::enabledValue() const {
	return _enabled.value();
}

bool AdminFolders::busy() const {
	return claimed();
}

rpl::producer<bool> AdminFolders::busyValue() const {
	return _busy.value();
}

void AdminFolders::load() {
	auto file = QFile(_path);
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	auto error = QJsonParseError();
	const auto document = QJsonDocument::fromJson(file.readAll(), &error);
	file.close();
	if (error.error != QJsonParseError::NoError || !document.isObject()) {
		return;
	}
	const auto root = document.object();
	// The state of another user, if this folder was ever reused, would
	// point at folder ids of somebody else's account: drop it.
	const auto user = QString::number(_session->userId().bare);
	if (root.value(u"user"_q).toString() != user
		|| root.value(u"format"_q).toInt() != kFormat) {
		QFile::remove(_path);
		return;
	}
	const auto kinds = root.value(u"kinds"_q).toArray();
	for (auto i = 0; i != kKindCount && i < kinds.size(); ++i) {
		const auto kind = kinds[i].toObject();
		_ids[i] = FilterId(std::max(kind.value(u"id"_q).toInt(), 0));
		_dismissed[i] = kind.value(u"dismissed"_q).toBool();
		for (const auto &value : kind.value(u"snapshot"_q).toArray()) {
			auto ok = false;
			const auto raw = value.toString().toULongLong(&ok);
			if (const auto id = ok ? DeserializePeerId(raw) : PeerId()) {
				_snapshot[i].emplace(id);
			}
		}
	}
	refreshEnabled();
}

void AdminFolders::save() {
	if (!ranges::any_of(_ids, [](FilterId id) { return id != 0; })) {
		clearState();
		return;
	}
	auto kinds = QJsonArray();
	for (auto i = 0; i != kKindCount; ++i) {
		auto snapshot = QJsonArray();
		for (const auto &id : _snapshot[i]) {
			snapshot.push_back(QString::number(SerializePeerId(id)));
		}
		kinds.push_back(QJsonObject{
			{ u"id"_q, int(_ids[i]) },
			{ u"dismissed"_q, _dismissed[i] },
			{ u"snapshot"_q, snapshot },
		});
	}
	const auto root = QJsonObject{
		{ u"format"_q, kFormat },
		{ u"user"_q, QString::number(_session->userId().bare) },
		{ u"kinds"_q, kinds },
	};
	auto file = QSaveFile(_path);
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
		if (!file.commit()) {
			LOG(("Novagram Error: could not write admin folders state."));
		}
	}
	refreshEnabled();
}

void AdminFolders::clearState() {
	_ids = {};
	_dismissed = {};
	for (auto &snapshot : _snapshot) {
		snapshot.clear();
	}
	QFile::remove(_path);
	refreshEnabled();
}

void AdminFolders::refreshEnabled() {
	_enabled = ranges::any_of(_ids, [](FilterId id) { return id != 0; });
}

bool AdminFolders::ready() const {
	// Folders come before the chats, and with the chats not there yet every
	// bucket looks empty: a sync would read that as all rights lost.
	const auto data = &_session->data();
	return data->chatsFilters().loaded()
		&& data->chatsListLoaded(nullptr)
		&& !data->chatsList(nullptr)->indexed()->empty();
}

bool AdminFolders::claimed() const {
	return _busy.current() && (crl::now() - _busySince < kBusyStale);
}

void AdminFolders::claim() {
	++_generation;
	_busySince = crl::now();
	_busy = true;
}

void AdminFolders::release() {
	_busy = false;
	// Whatever changed while this ran was not looked at.
	scheduleSync();
}

void AdminFolders::scheduleSync() {
	if (!enabled() || _syncTimer.isActive()) {
		return;
	}
	const auto passed = crl::now() - _lastSync;
	_syncTimer.callOnce(std::max(kSyncDelay, kSyncInterval - passed));
}

void AdminFolders::sync() {
	// Never alongside enable / refresh / disable: mid-operation the ids
	// are only partly recorded, and a kind still in flight would look like
	// one without a folder and be made a second time.
	if (!enabled() || claimed() || !ready()) {
		return;
	}
	_lastSync = crl::now();

	const auto buckets = classify();
	const auto perFolder = chatsLimit();
	auto stateChanged = false;
	auto updates = std::vector<std::pair<Kind, std::vector<PeerId>>>();
	auto fresh = std::vector<Kind>();
	for (auto i = 0; i != kKindCount; ++i) {
		const auto kind = Kind(i);
		const auto &now = buckets[i];
		if (!_ids[i]) {
			if (!now.empty() && !_dismissed[i]) {
				fresh.push_back(kind);
			}
			continue;
		}
		const auto existing = tracked(kind);
		if (!existing) {
			// Deleted by hand or on another device (the folders are loaded,
			// so missing means gone): forget it rather than bring back
			// something the user got rid of on purpose.
			_ids[i] = 0;
			_snapshot[i].clear();
			_dismissed[i] = true;
			stateChanged = true;
			continue;
		}
		const auto nowSet = base::flat_set<PeerId>(now.begin(), now.end());
		auto next = nowSet;
		auto removed = std::vector<PeerId>();
		for (const auto &id : _snapshot[i]) {
			if (nowSet.contains(id)) {
				continue;
			}
			// Not among the loaded chats with this kind: removed only once
			// the chat itself says so; an archive not loaded yet or a peer
			// not known yet must not empty the folder.
			const auto peer = _session->data().peerLoaded(id);
			const auto classified = peer
				? classify(not_null(peer))
				: Classified();
			if (classified.known && classified.kind != kind) {
				removed.push_back(id);
			} else {
				next.emplace(id);
			}
		}
		auto added = std::vector<PeerId>();
		for (const auto &id : now) {
			if (!_snapshot[i].contains(id)) {
				added.push_back(id);
			}
		}
		if (next != _snapshot[i]) {
			_snapshot[i] = std::move(next);
			stateChanged = true;
		}
		if (added.empty() && removed.empty()) {
			continue;
		}
		// Apply only the delta onto what the folder holds now: chats the
		// user put there or took out by hand stay as they are.
		auto members = std::vector<PeerId>();
		auto have = base::flat_set<PeerId>();
		for (const auto &history : existing->always()) {
			const auto id = history->peer->id;
			if (!ranges::contains(removed, id)) {
				members.push_back(id);
				have.emplace(id);
			}
		}
		for (const auto &id : added) {
			if (int(members.size()) >= perFolder) {
				break;
			} else if (have.emplace(id).second) {
				members.push_back(id);
			}
		}
		updates.emplace_back(kind, std::move(members));
	}

	// A new folder only while there is room (one of ours already on the
	// account costs none); a refused request from a background check the
	// user did not ask for would be noise.
	auto room = foldersLimit() - foldersCount();
	auto creates = std::vector<Kind>();
	{
		auto taken = base::flat_set<FilterId>();
		for (const auto kind : fresh) {
			if (const auto adopt = adoptable(kind, taken)) {
				taken.emplace(adopt->id());
				creates.push_back(kind);
			} else if (room > 0) {
				--room;
				creates.push_back(kind);
			}
		}
	}

	if (stateChanged) {
		save();
	}
	if (!enabled()) {
		// Every folder of ours was deleted: the feature is off now, and
		// nothing is made again on its own.
		return;
	} else if (updates.empty() && creates.empty()) {
		return;
	}
	claim();

	auto steps = std::vector<Step>();
	for (auto &update : updates) {
		const auto kind = update.first;
		auto members = std::move(update.second);
		steps.push_back([=, members = std::move(members)](Fn<void()> next) {
			const auto existing = tracked(kind);
			if (!existing || members.empty()) {
				// An empty folder with no rules is refused
				// (FILTER_INCLUDE_EMPTY), and would empty one the user
				// relies on: leave it as it is.
				next();
				return;
			}
			auto filter = makeFilter(
				kind,
				existing->id(),
				existing,
				members,
				false);
			if (filter == *existing) {
				next();
				return;
			}
			saveFilter(std::move(filter), [=](bool) { next(); });
		});
	}
	const auto taken = std::make_shared<base::flat_set<FilterId>>();
	for (const auto kind : creates) {
		steps.push_back([=](Fn<void()> next) {
			const auto i = int(kind);
			if (_ids[i]) {
				next();
				return;
			}
			auto members = buckets[i];
			if (int(members.size()) > perFolder) {
				members.resize(perFolder);
			}
			if (members.empty()) {
				next();
				return;
			}
			const auto adopt = adoptable(kind, *taken);
			const auto id = adopt ? adopt->id() : freeId(*taken);
			taken->emplace(id);
			auto filter = makeFilter(kind, id, adopt, members, true);
			saveFilter(std::move(filter), [=](bool ok) {
				if (ok || adopt) {
					_ids[i] = id;
					_snapshot[i] = base::flat_set<PeerId>(
						buckets[i].begin(),
						buckets[i].end());
					save();
				}
				next();
			});
		});
	}
	runChain(std::move(steps), [=] {
		release();
	});
}

AdminFolders::Buckets AdminFolders::classify() const {
	auto result = Buckets();
	auto seen = base::flat_set<PeerId>();
	const auto add = [&](not_null<Dialogs::MainList*> list) {
		for (const auto &row : list->indexed()->all()) {
			const auto history = row->history();
			if (!history) {
				continue;
			}
			const auto peer = history->peer;
			if ((!peer->isChat() && !peer->isChannel())
				|| !seen.emplace(peer->id).second) {
				continue;
			}
			if (const auto kind = classify(peer).kind) {
				result[int(*kind)].push_back(peer->id);
			}
		}
	};
	const auto data = &_session->data();
	add(data->chatsList(nullptr));
	if (const auto folder = data->folderLoaded(Data::Folder::kId)) {
		add(folder->chatsList());
	}
	return result;
}

AdminFolders::Classified AdminFolders::classify(
		not_null<PeerData*> peer) const {
	if (!peer->isLoaded()) {
		return {};
	}
	// Owners stay out of the admin kinds: a creator holds admin rights too,
	// and "My groups" and "Admin group" would come out near the same.
	const auto kindOf = [](bool broadcast, bool owner) {
		return broadcast
			? (owner ? Kind::ChannelOwner : Kind::ChannelAdmin)
			: (owner ? Kind::GroupOwner : Kind::GroupAdmin);
	};
	if (const auto channel = peer->asChannel()) {
		// Left, kicked or closed: rights there mean nothing.
		if (!channel->amIn() || channel->isMonoforum()) {
			return { .known = true };
		}
		const auto owner = channel->amCreator();
		if (!owner && !channel->hasAdminRights()) {
			return { .known = true };
		}
		// Not a megagroup is a channel; everything else is a group, which
		// is how people think about it (and how Android sorts them).
		return {
			.known = true,
			.kind = kindOf(!channel->isMegagroup(), owner),
		};
	} else if (const auto chat = peer->asChat()) {
		if (!chat->amIn() || chat->isMigrated()) {
			return { .known = true };
		}
		const auto owner = chat->amCreator();
		if (!owner && !chat->hasAdminRights()) {
			return { .known = true };
		}
		return { .known = true, .kind = kindOf(false, owner) };
	}
	return { .known = true };
}

const Data::ChatFilter *AdminFolders::filterById(FilterId id) const {
	if (!id) {
		return nullptr;
	}
	const auto &list = _session->data().chatsFilters().list();
	const auto i = ranges::find(list, id, &Data::ChatFilter::id);
	return (i != end(list)) ? &*i : nullptr;
}

const Data::ChatFilter *AdminFolders::tracked(Kind kind) const {
	const auto filter = filterById(_ids[int(kind)]);
	return (filter && Usable(*filter)) ? filter : nullptr;
}

const Data::ChatFilter *AdminFolders::adoptable(
		Kind kind,
		const base::flat_set<FilterId> &taken) const {
	for (const auto &filter : _session->data().chatsFilters().list()) {
		const auto id = filter.id();
		if (Usable(filter)
			&& !ranges::contains(_ids, id)
			&& !taken.contains(id)
			&& IsTitleOf(kind, filter.titleText().text)) {
			return &filter;
		}
	}
	return nullptr;
}

FilterId AdminFolders::freeId(const base::flat_set<FilterId> &taken) const {
	// The allocation Telegram's own folder editor uses.
	auto id = FilterId(2);
	while (filterById(id) || taken.contains(id)) {
		++id;
	}
	return id;
}

int AdminFolders::foldersCount() const {
	return int(ranges::count_if(
		_session->data().chatsFilters().list(),
		[](const Data::ChatFilter &filter) { return filter.id() != 0; }));
}

int AdminFolders::foldersLimit() const {
	return Data::PremiumLimits(_session).dialogFiltersCurrent();
}

int AdminFolders::chatsLimit() const {
	const auto limits = Data::PremiumLimits(_session);
	return std::max(limits.dialogFiltersChatsCurrent(), 1);
}

QString AdminFolders::limitText() const {
	return tr::lng_fenix_admin_folders_limit(
		tr::now,
		lt_limit,
		QString::number(foldersLimit()),
		lt_have,
		QString::number(foldersCount()));
}

Data::ChatFilter AdminFolders::makeFilter(
		Kind kind,
		FilterId id,
		const Data::ChatFilter *existing,
		const std::vector<PeerId> &members,
		bool rebuild) const {
	const auto owner = &_session->data();
	auto always = base::flat_set<not_null<History*>>();
	for (const auto &peerId : members) {
		always.emplace(owner->history(peerId));
	}
	// What Telegram's own editor keeps when the chats change: nothing both
	// included and excluded, nothing pinned that is not included. The
	// exclusions and pins of the user inside our folders are kept.
	auto pinned = std::vector<not_null<History*>>();
	auto never = base::flat_set<not_null<History*>>();
	if (existing) {
		for (const auto &history : existing->pinned()) {
			if (always.contains(history)) {
				pinned.push_back(history);
			}
		}
		for (const auto &history : existing->never()) {
			if (!always.contains(history)) {
				never.emplace(history);
			}
		}
	}
	if (rebuild || !existing) {
		// No rules: the folder holds exactly these chats.
		return Data::ChatFilter(
			id,
			{ .text = { TitleFor(kind) }, .isStatic = false },
			existing ? existing->iconEmoji() : QString(),
			uint8(int(kind) % 8),
			Data::ChatFilter::Flags(),
			std::move(always),
			std::move(pinned),
			std::move(never));
	}
	return Data::ChatFilter(
		id,
		existing->title(),
		existing->iconEmoji(),
		existing->colorIndex(),
		existing->flags(),
		std::move(always),
		std::move(pinned),
		std::move(never));
}

void AdminFolders::saveFilter(Data::ChatFilter filter, Fn<void(bool)> done) {
	const auto id = filter.id();
	const auto tl = filter.tl();
	_session->api().request(MTPmessages_UpdateDialogFilter(
		MTP_flags(MTPmessages_UpdateDialogFilter::Flag::f_filter),
		MTP_int(id),
		tl
	)).done(crl::guard(this, [=] {
		_session->data().chatsFilters().apply(MTP_updateDialogFilter(
			MTP_flags(MTPDupdateDialogFilter::Flag::f_filter),
			MTP_int(id),
			tl));
		done(true);
	})).fail(crl::guard(this, [=](const MTP::Error &error) {
		LOG(("Novagram Error: could not save admin folder %1: %2"
			).arg(id
			).arg(error.type()));
		done(false);
	})).send();
}

void AdminFolders::deleteFilter(FilterId id, Fn<void(bool)> done) {
	const auto filter = filterById(id);
	if (!filter || !Usable(*filter)) {
		// Already gone (deleted by hand, or on another device): the caller
		// wanted it gone, nothing to undo.
		done(true);
		return;
	}
	_session->api().request(MTPmessages_UpdateDialogFilter(
		MTP_flags(MTPmessages_UpdateDialogFilter::Flag(0)),
		MTP_int(id),
		MTPDialogFilter()
	)).done(crl::guard(this, [=] {
		_session->data().chatsFilters().apply(MTP_updateDialogFilter(
			MTP_flags(MTPDupdateDialogFilter::Flag(0)),
			MTP_int(id),
			MTPDialogFilter()));
		done(true);
	})).fail(crl::guard(this, [=](const MTP::Error &error) {
		LOG(("Novagram Error: could not delete admin folder %1: %2"
			).arg(id
			).arg(error.type()));
		done(false);
	})).send();
}

void AdminFolders::runChain(std::vector<Step> steps, Fn<void()> finished) {
	// Strictly one request after another: parallel saves race on the
	// order of the folders on the server.
	const auto running = std::make_shared<Running>(Running{
		.steps = std::move(steps),
		.finished = std::move(finished),
		.generation = _generation,
	});
	runStep(running, 0);
}

void AdminFolders::runStep(std::shared_ptr<Running> running, int index) {
	if (running->generation != _generation) {
		// Taken over after going stale: its reply came too late to matter.
		return;
	} else if (index >= int(running->steps.size())) {
		running->finished();
		return;
	}
	running->steps[index](crl::guard(this, [=] {
		runStep(running, index + 1);
	}));
}

void AdminFolders::enable(Fn<void(Result)> done) {
	if (claimed()) {
		return;
	} else if (!ready()) {
		done({ .blocked = tr::lng_fenix_admin_folders_loading(tr::now) });
		return;
	}
	const auto buckets = classify();
	auto kinds = std::vector<Kind>();
	for (auto i = 0; i != kKindCount; ++i) {
		if (!buckets[i].empty()) {
			kinds.push_back(Kind(i));
		}
	}
	if (kinds.empty()) {
		done({ .blocked = tr::lng_fenix_admin_folders_none(tr::now) });
		return;
	}

	// Charge the limit only for folders that are really new: one of ours
	// already on the account (made by Android, or before) gets adopted.
	auto needSlots = 0;
	{
		auto taken = base::flat_set<FilterId>();
		for (const auto kind : kinds) {
			if (const auto has = tracked(kind)) {
				taken.emplace(has->id());
			} else if (const auto adopt = adoptable(kind, taken)) {
				taken.emplace(adopt->id());
			} else {
				++needSlots;
			}
		}
	}
	if (foldersLimit() - foldersCount() < needSlots) {
		done({ .blocked = limitText() });
		return;
	}

	claim();
	_dismissed = {};

	const auto perFolder = chatsLimit();
	const auto result = std::make_shared<Result>();
	const auto taken = std::make_shared<base::flat_set<FilterId>>();
	auto steps = std::vector<Step>();
	for (const auto kind : kinds) {
		steps.push_back([=](Fn<void()> next) {
			const auto i = int(kind);
			auto members = buckets[i];
			if (int(members.size()) > perFolder) {
				members.resize(perFolder);
				result->truncated.emplace_back(TitleFor(kind), perFolder);
			}
			const auto has = tracked(kind);
			const auto existing = has ? has : adoptable(kind, *taken);
			const auto id = existing ? existing->id() : freeId(*taken);
			taken->emplace(id);
			auto filter = makeFilter(kind, id, existing, members, true);
			const auto record = [=](bool ok) {
				if (!ok) {
					result->failed = true;
				}
				// One of ours that is already there stays ours even when
				// this rewrite of it failed.
				if (ok || existing) {
					_ids[i] = id;
					_snapshot[i] = base::flat_set<PeerId>(
						buckets[i].begin(),
						buckets[i].end());
					save();
				}
				next();
			};
			if (existing && filter == *existing) {
				record(true);
			} else {
				saveFilter(std::move(filter), record);
			}
		});
	}
	runChain(std::move(steps), [=] {
		release();
		done(*result);
	});
}

void AdminFolders::refresh(Fn<void(Result)> done) {
	if (claimed()) {
		return;
	} else if (!enabled()) {
		// Nothing of ours on record: a first build, not a repair.
		enable(std::move(done));
		return;
	} else if (!ready()) {
		// A half-loaded chat list would read as "you administer nothing"
		// and delete all four.
		done({ .blocked = tr::lng_fenix_admin_folders_loading(tr::now) });
		return;
	}
	const auto buckets = classify();
	const auto perFolder = chatsLimit();

	// Decided before a single request goes out, so the chain does not
	// change its mind on a list its own writes are changing.
	auto updates = std::vector<Kind>();
	auto creates = std::vector<Kind>();
	auto deletes = std::vector<Kind>();
	for (auto i = 0; i != kKindCount; ++i) {
		const auto kind = Kind(i);
		const auto has = !buckets[i].empty();
		if (tracked(kind)) {
			(has ? updates : deletes).push_back(kind);
		} else if (has) {
			// Refresh is an explicit "rebuild from my rights": unlike the
			// background sync it brings back a folder deleted by hand.
			creates.push_back(kind);
		} else {
			_ids[i] = 0;
			_snapshot[i].clear();
		}
	}
	_dismissed = {};

	// Over the limit the creates that need a new slot are skipped and the
	// rest still runs: a partial repair beats none.
	auto room = foldersLimit() - foldersCount();
	auto creatable = std::vector<Kind>();
	{
		auto taken = base::flat_set<FilterId>();
		for (const auto kind : creates) {
			if (const auto adopt = adoptable(kind, taken)) {
				taken.emplace(adopt->id());
				creatable.push_back(kind);
			} else if (room > 0) {
				--room;
				creatable.push_back(kind);
			}
		}
	}
	const auto skipped = (creatable.size() != creates.size());

	claim();
	save();

	const auto result = std::make_shared<Result>();
	const auto taken = std::make_shared<base::flat_set<FilterId>>();
	const auto write = [=](Kind kind, Fn<void()> next) {
		const auto i = int(kind);
		auto members = buckets[i];
		if (int(members.size()) > perFolder) {
			members.resize(perFolder);
			result->truncated.emplace_back(TitleFor(kind), perFolder);
		}
		const auto has = tracked(kind);
		const auto existing = has ? has : adoptable(kind, *taken);
		const auto id = existing ? existing->id() : freeId(*taken);
		taken->emplace(id);
		// Rewriting the title too repairs one carrying another language's
		// or an older name.
		auto filter = makeFilter(kind, id, existing, members, true);
		const auto record = [=](bool ok) {
			if (!ok) {
				result->failed = true;
			}
			if (ok || existing) {
				_ids[i] = id;
				_snapshot[i] = base::flat_set<PeerId>(
					buckets[i].begin(),
					buckets[i].end());
				save();
			}
			next();
		};
		if (existing && filter == *existing) {
			record(true);
		} else {
			saveFilter(std::move(filter), record);
		}
	};
	auto steps = std::vector<Step>();
	for (const auto kind : updates) {
		steps.push_back([=](Fn<void()> next) { write(kind, next); });
	}
	for (const auto kind : creatable) {
		steps.push_back([=](Fn<void()> next) { write(kind, next); });
	}
	// Deletions run last: a chain broken halfway then leaves one stale
	// folder, not an account with none.
	for (const auto kind : deletes) {
		steps.push_back([=](Fn<void()> next) {
			const auto i = int(kind);
			deleteFilter(_ids[i], [=](bool ok) {
				if (ok) {
					_ids[i] = 0;
					_snapshot[i].clear();
					save();
				} else {
					result->failed = true;
				}
				next();
			});
		});
	}
	runChain(std::move(steps), [=] {
		if (skipped) {
			result->blocked = limitText();
		}
		release();
		done(*result);
	});
}

void AdminFolders::disable(Fn<void()> done) {
	if (claimed()) {
		return;
	}
	// The tracked ones and also any other folder under one of our titles:
	// whatever our bookkeeping lost (another device made it, the state was
	// cleared) would otherwise be left for the user to delete by hand.
	auto ids = std::vector<FilterId>();
	for (const auto id : _ids) {
		if (id) {
			ids.push_back(id);
		}
	}
	for (const auto &filter : _session->data().chatsFilters().list()) {
		if (Usable(filter)
			&& IsOurTitle(filter.titleText().text)
			&& !ranges::contains(ids, filter.id())) {
			ids.push_back(filter.id());
		}
	}
	claim();

	const auto failed = std::make_shared<base::flat_set<FilterId>>();
	auto steps = std::vector<Step>();
	for (const auto id : ids) {
		steps.push_back([=](Fn<void()> next) {
			deleteFilter(id, [=](bool ok) {
				if (!ok) {
					failed->emplace(id);
				}
				next();
			});
		});
	}
	runChain(std::move(steps), [=] {
		// Keep tracking what the server refused to delete: forgotten, it
		// would come back with the next folders update as a stray that
		// the next enable could not tell from a second set.
		for (auto i = 0; i != kKindCount; ++i) {
			if (!failed->contains(_ids[i])) {
				_ids[i] = 0;
				_snapshot[i].clear();
			}
		}
		_dismissed = {};
		save();
		release();
		done();
	});
}

void SetupAdminFolders() {
	static auto lifetime = rpl::lifetime();
	Core::App().domain().activeSessionValue(
	) | rpl::on_next([](Main::Session *session) {
		if (session) {
			// Made on first use, it lives as long as the session does.
			[[maybe_unused]] const auto &folders = GetAdminFolders(session);
		}
	}, lifetime);
}

AdminFolders &GetAdminFolders(not_null<Main::Session*> session) {
	auto &stored = Stored();
	const auto i = stored.find(session);
	if (i != end(stored)) {
		return *i->second;
	}
	auto made = std::make_unique<AdminFolders>(session);
	const auto result = made.get();
	stored.emplace(session, std::move(made));
	session->lifetime().add([=] {
		Stored().remove(session);
	});
	return *result;
}

} // namespace Fenix
