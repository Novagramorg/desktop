/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/kept_messages.h"

#include "base/unixtime.h"
#include "data/data_session.h"
#include "fenix/deleted_msg.h"
#include "fenix/fenix_settings.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/view/history_view_element.h"
#include "main/main_session.h"

#include <QtCore/QDataStream>

namespace Fenix {
namespace {

constexpr auto kSaveDelay = 3 * crl::time(1000);

// The copies kept longest ago go first when a chat has more.
constexpr auto kMaxPerChat = 1000;

// A message with more data than this is not kept.
constexpr auto kMaxCopySize = 256 * 1024;

[[nodiscard]] QString KeptFileName(PeerId peerId) {
	return QString::fromLatin1(kKeptFilePrefix)
		+ QString::number(SerializePeerId(peerId), 16);
}

[[nodiscard]] QByteArray Serialize(const MTPMessage &message) {
	auto buffer = mtpBuffer();
	message.write(buffer);
	return QByteArray(
		reinterpret_cast<const char*>(buffer.constData()),
		buffer.size() * sizeof(mtpPrime));
}

[[nodiscard]] FullMsgId MessageId(const MTPMessage &message) {
	return message.match([](const MTPDmessage &data) {
		return FullMsgId(peerFromMTP(data.vpeer_id()), data.vid().v);
	}, [](const auto &) {
		return FullMsgId();
	});
}

[[nodiscard]] TimeId MessageDate(const MTPMessage &message) {
	return message.match([](const MTPDmessage &data) {
		return data.vdate().v;
	}, [](const auto &) {
		return TimeId();
	});
}

// The chat's auto-delete removed it by now: as in Telegram, it is gone for
// good, also when it was kept before (Android HistoryDb does the same).
[[nodiscard]] bool AutoDeleted(const MTPMessage &message) {
	return message.match([](const MTPDmessage &data) {
		const auto period = data.vttl_period();
		return period
			&& (period->v > 0)
			&& (data.vdate().v + period->v <= base::unixtime::now());
	}, [](const auto &) {
		return false;
	});
}

// Restoring creates items: nothing it starts may restore again meanwhile.
auto Restoring = false;

// A kept message shows again with its "Deleted" mark.
void EnsureDeletedMark(
		not_null<Main::Session*> session,
		PeerId peerId,
		MsgId msgId) {
	auto &deleted = GetDeleted(session);
	if (!deleted.whoDeleted(peerId, msgId)) {
		deleted.recordDeletion(
			peerId,
			msgId,
			peerIsChannel(peerId) ? DeletedBy::Channel : DeletedBy::Other);
	}
}

} // namespace

std::optional<MTPMessage> ParseKept(const QByteArray &data) {
	if (data.isEmpty() || (data.size() % sizeof(mtpPrime))) {
		return std::nullopt;
	}
	auto buffer = mtpBuffer();
	buffer.resize(data.size() / sizeof(mtpPrime));
	memcpy(buffer.data(), data.constData(), data.size());
	const mtpPrime *from = buffer.constData();
	const auto end = from + buffer.size();
	auto result = MTPMessage();
	if (!result.read(from, end) || from != end) {
		return std::nullopt;
	}
	return result;
}

void RememberLoaded(
		not_null<Main::Session*> session,
		const MTPMessage &message) {
	if (Get().saveDeletedMsg()) {
		GetKept(session).remember(message);
	}
}

KeptMessages::KeptMessages(not_null<Main::Session*> session)
: _session(session)
, _saveTimer([=] { saveDirty(); }) {
	session->data().itemRemoved(
	) | rpl::on_next([=](not_null<const HistoryItem*> item) {
		_loaded.erase(item->fullId());
	}, _lifetime);

	Get().saveDeletedMsgChanges(
	) | rpl::filter([](bool enabled) {
		return !enabled;
	}) | rpl::on_next([=] {
		_loaded.clear();
	}, _lifetime);
}

KeptMessages::~KeptMessages() {
	if (!_dirty.empty()) {
		saveDirty();
	}
}

void KeptMessages::remember(const MTPMessage &message) {
	if (_wiped) {
		return;
	}
	const auto id = MessageId(message);
	if (!id || !IsServerMsgId(id.msg)) {
		return;
	}
	auto data = Serialize(message);
	if (data.size() <= kMaxCopySize) {
		_loaded[id] = std::move(data);
	}
}

void KeptMessages::keep(not_null<HistoryItem*> item) {
	if (_wiped) {
		return;
	}
	const auto i = _loaded.find(item->fullId());
	if (i == end(_loaded)) {
		// Loaded before Save Deleted Messages was on: only its mark stays.
		return;
	}
	const auto peerId = item->history()->peer->id;
	auto &list = chat(peerId).list;
	const auto id = item->id;
	const auto j = ranges::lower_bound(list, id, ranges::less(), &Kept::id);
	if (j != end(list) && j->id == id) {
		j->data = i->second;
	} else {
		list.insert(j, Kept{
			.id = id,
			.at = base::unixtime::now(),
			.data = i->second,
		});
		if (int(list.size()) > kMaxPerChat) {
			list.erase(ranges::min_element(list, ranges::less(), &Kept::at));
		}
	}
	changed(peerId);
}

const std::vector<KeptMessages::Kept> &KeptMessages::list(PeerId peerId) {
	return chat(peerId).list;
}

void KeptMessages::forget(PeerId peerId, MsgId msgId) {
	auto &list = chat(peerId).list;
	const auto i = ranges::find(list, msgId, &Kept::id);
	if (i != end(list)) {
		list.erase(i);
		changed(peerId);
	}
}

void KeptMessages::forgetChat(PeerId peerId) {
	auto &list = chat(peerId).list;
	if (!list.empty()) {
		list.clear();
		changed(peerId);
	}
}

std::vector<MsgId> KeptMessages::forgetBetween(
		PeerId peerId,
		TimeId minDate,
		TimeId maxDate) {
	auto result = std::vector<MsgId>();
	auto &list = chat(peerId).list;
	const auto removed = ranges::remove_if(list, [&](const Kept &kept) {
		const auto message = ParseKept(kept.data);
		const auto date = message ? MessageDate(*message) : TimeId();
		if ((date > minDate) && (date < maxDate)) {
			result.push_back(kept.id);
			return true;
		}
		return false;
	});
	if (removed != end(list)) {
		list.erase(removed, end(list));
		changed(peerId);
	}
	return result;
}

void KeptMessages::clear() {
	if (_wiped) {
		return;
	}
	auto peers = std::vector<PeerId>();
	for (const auto &[peerId, chat] : _chats) {
		if (!chat.list.empty()) {
			peers.push_back(peerId);
		}
	}
	_saveTimer.cancel();
	_dirty.clear();
	_chats.clear();
	RemoveSessionFiles(_session, QString::fromLatin1(kKeptFilePrefix));
	for (const auto peerId : peers) {
		_changes.fire_copy(peerId);
	}
}

void KeptMessages::wipe() {
	_wiped = true;
	_saveTimer.cancel();
	_dirty.clear();
	_chats.clear();
	_loaded.clear();
}

rpl::producer<PeerId> KeptMessages::changes() const {
	return _changes.events();
}

KeptMessages::Chat &KeptMessages::chat(PeerId peerId) {
	auto &result = _chats[peerId];
	if (!result.loaded) {
		result.loaded = true;
		if (!_wiped) {
			load(peerId, result);
		}
	}
	return result;
}

SessionFile KeptMessages::file(PeerId peerId) const {
	return SessionFile(_session, KeptFileName(peerId));
}

void KeptMessages::changed(PeerId peerId) {
	if (!_wiped) {
		_dirty.emplace(peerId);
		if (!_saveTimer.isActive()) {
			_saveTimer.callOnce(kSaveDelay);
		}
	}
	_changes.fire_copy(peerId);
}

void KeptMessages::load(PeerId peerId, Chat &chat) {
	const auto payload = file(peerId).read();
	if (payload.isEmpty()) {
		return;
	}
	QDataStream stream(payload);
	stream.setVersion(QDataStream::Qt_5_1);
	auto count = quint32();
	stream >> count;
	if (stream.status() != QDataStream::Ok
		|| count > quint32(4 * kMaxPerChat)) {
		return;
	}
	auto list = std::vector<Kept>();
	list.reserve(count);
	auto autoDeleted = std::vector<MsgId>();
	for (auto i = quint32(); i != count; ++i) {
		auto id = qint64();
		auto at = qint32();
		auto data = QByteArray();
		stream >> id >> at >> data;
		if (stream.status() != QDataStream::Ok) {
			return;
		}
		if (!IsServerMsgId(MsgId(id)) || data.isEmpty()) {
			continue;
		}
		const auto message = ParseKept(data);
		if (message && AutoDeleted(*message)) {
			autoDeleted.push_back(MsgId(id));
			continue;
		}
		list.push_back({ .id = MsgId(id), .at = TimeId(at), .data = data });
	}
	ranges::sort(list, ranges::less(), &Kept::id);
	list.erase(
		ranges::unique(list, ranges::equal_to(), &Kept::id),
		end(list));
	chat.list = std::move(list);

	if (!autoDeleted.empty()) {
		// Written again without them. Not through changed(): this runs
		// while the chat's list is being read.
		auto &deleted = GetDeleted(_session);
		for (const auto id : autoDeleted) {
			deleted.forget(peerId, id);
		}
		_dirty.emplace(peerId);
		if (!_saveTimer.isActive()) {
			_saveTimer.callOnce(kSaveDelay);
		}
	}
}

void KeptMessages::saveDirty() {
	if (_wiped) {
		return;
	}
	for (const auto peerId : base::take(_dirty)) {
		const auto i = _chats.find(peerId);
		if (i != end(_chats)) {
			save(peerId, i->second);
		}
	}
}

void KeptMessages::save(PeerId peerId, const Chat &chat) {
	auto payload = QByteArray();
	{
		QDataStream stream(&payload, QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_5_1);
		stream << quint32(chat.list.size());
		for (const auto &kept : chat.list) {
			stream
				<< qint64(kept.id.bare)
				<< qint32(kept.at)
				<< kept.data;
		}
	}
	file(peerId).write(payload);
}

HistoryItem *KeptHistory::Item(
		not_null<History*> history,
		const KeptMessages::Kept &kept) {
	if (const auto existing = history->owner().message(history->peer, kept.id)) {
		return existing;
	}
	const auto message = ParseKept(kept.data);
	if (!message
		|| MessageId(*message) != FullMsgId(history->peer->id, kept.id)
		|| AutoDeleted(*message)) {
		// An auto-deleted one is forgotten when the chat's file is read.
		return nullptr;
	}
	return history->createItem(kept.id, *message, MessageFlags());
}

void KeptHistory::Restore(not_null<History*> history) {
	if (Restoring
		|| !Get().saveDeletedMsg()
		|| history->isBuildingFrontBlock()) {
		return;
	}
	const auto session = &history->session();
	const auto peerId = history->peer->id;

	// A copy: creating the items may not change the list, but stays safe.
	const auto list = GetKept(session).list(peerId);
	if (list.empty()) {
		return;
	}
	const auto empty = history->isEmpty();
	const auto top = history->loadedAtTop();
	const auto bottom = history->loadedAtBottom();
	if (empty && !(top && bottom)) {
		return;
	}
	const auto min = history->minMsgId();
	const auto max = history->maxMsgId();

	Restoring = true;
	const auto guard = gsl::finally([] { Restoring = false; });
	for (const auto &kept : list) {
		// Only where the history around it is loaded, so it never shows
		// out of its place.
		if (!empty
			&& ((kept.id < min && !top) || (kept.id > max && !bottom))) {
			continue;
		}
		const auto item = Item(history, kept);
		if (!item || item->mainView() || item->history() != history) {
			continue;
		}
		EnsureDeletedMark(session, peerId, kept.id);
		Insert(history, item);
	}
}

void KeptHistory::AddToSlice(
		not_null<History*> history,
		std::vector<MsgId> &ids,
		MsgRange range,
		int *fullCount) {
	if (Restoring || !Get().saveDeletedMsg()) {
		return;
	}
	const auto session = &history->session();
	const auto peerId = history->peer->id;

	// A copy: creating the items may not change the list, but stays safe.
	const auto list = GetKept(session).list(peerId);
	if (list.empty()) {
		return;
	}

	// The server counts only the messages it still has. The view takes
	// the chat as fully loaded once it holds that many, so the kept ones
	// count too: at most a few more requests, never a missing part.
	if (fullCount) {
		// The server may report INT_MAX even for a partial history.
		*fullCount = int(std::min(
			int64(*fullCount) + int64(list.size()),
			int64(std::numeric_limits<int>::max())));
	}

	Restoring = true;
	const auto guard = gsl::finally([] { Restoring = false; });
	for (const auto &kept : list) {
		// The range has every message between its ends, so a kept one in
		// it is in its place there.
		if (kept.id < range.from || kept.id > range.till) {
			continue;
		}
		const auto item = Item(history, kept);
		if (!item || item->history() != history) {
			continue;
		}
		EnsureDeletedMark(session, peerId, kept.id);
		if (!ranges::contains(ids, kept.id)) {
			ids.push_back(kept.id);
		}
	}
}

void KeptHistory::Insert(
		not_null<History*> history,
		not_null<HistoryItem*> item) {
	// Before the first loaded server message with a greater id, else right
	// after the last one: where the message was before it was deleted.
	auto lastBlock = -1;
	auto lastIndex = -1;
	const auto &blocks = history->blocks;
	for (auto b = 0, bl = int(blocks.size()); b != bl; ++b) {
		const auto &messages = blocks[b]->messages;
		for (auto i = 0, il = int(messages.size()); i != il; ++i) {
			const auto id = messages[i]->data()->id;
			if (!IsServerMsgId(id)) {
				continue;
			} else if (id > item->id) {
				history->addNewInTheMiddle(item, b, i);
				return;
			}
			lastBlock = b;
			lastIndex = i;
		}
	}
	if (lastBlock >= 0) {
		history->addNewInTheMiddle(item, lastBlock, lastIndex + 1);
	} else {
		history->addNewToBack(item, false);
	}
}

} // namespace Fenix
