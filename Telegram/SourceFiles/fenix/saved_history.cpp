/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/saved_history.h"

#include "core/application.h"
#include "data/data_session.h"
#include "fenix/deleted_msg.h"
#include "fenix/edit_history.h"
#include "fenix/kept_messages.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "settings.h"
#include "storage/details/storage_file_utilities.h"
#include "storage/serialize_common.h"
#include "storage/storage_account.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Fenix {
namespace {

constexpr auto kFormat = quint32(1);

struct Stores {
	explicit Stores(not_null<Main::Session*> session)
	: deleted(session)
	, edits(session)
	, kept(session) {
	}

	DeletedMsg deleted;
	EditHistory edits;
	KeptMessages kept;
};

using Registry = base::flat_map<
	not_null<Main::Session*>,
	std::unique_ptr<Stores>>;

[[nodiscard]] Registry &Stored() {
	static auto result = Registry();
	return result;
}

[[nodiscard]] Stores *Find(not_null<Main::Session*> session) {
	const auto &stored = Stored();
	const auto i = stored.find(session);
	return (i != end(stored)) ? i->second.get() : nullptr;
}

void MigrateLegacy(not_null<Main::Session*> current);

[[nodiscard]] Stores &Resolve(not_null<Main::Session*> session) {
	if (const auto stores = Find(session)) {
		return *stores;
	}
	auto made = std::make_unique<Stores>(session);
	const auto result = made.get();
	Stored().emplace(session, std::move(made));

	// Pending changes are written when the session goes away.
	session->lifetime().add([=] {
		Stored().remove(session);
	});

	MigrateLegacy(session);
	return *result;
}

[[nodiscard]] QString SessionBasePath(not_null<Main::Session*> session) {
	// Next to the account's cache database: Clear cache leaves it alone.
	return QFileInfo(session->local().cachePath()).absolutePath() + '/';
}

[[nodiscard]] QString LegacyPath(const QString &name) {
	return cWorkingDir() + u"tdata/"_q + name;
}

[[nodiscard]] QJsonArray ReadLegacy(const QString &path) {
	auto file = QFile(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return QJsonArray();
	}
	auto error = QJsonParseError();
	const auto document = QJsonDocument::fromJson(file.readAll(), &error);
	return (error.error == QJsonParseError::NoError && document.isArray())
		? document.array()
		: QJsonArray();
}

[[nodiscard]] FullMsgId LegacyKey(const QJsonObject &object) {
	const auto peer = PeerId(
		object.value(u"peer"_q).toVariant().toULongLong());
	const auto msg = MsgId(object.value(u"msg"_q).toVariant().toLongLong());
	return (peer && IsServerMsgId(msg)) ? FullMsgId(peer, msg) : FullMsgId();
}

// Before, all accounts shared plain files tdata/fenix_deleted.json and
// tdata/fenix_edits.json. They move into the accounts' own files once, when
// the first account's saved history is opened (by then every account that
// is logged in has its session), and are removed.
void MigrateLegacy(not_null<Main::Session*> current) {
	static auto done = false;
	if (done) {
		return;
	}
	done = true;

	const auto deletedPath = LegacyPath(u"fenix_deleted.json"_q);
	const auto editsPath = LegacyPath(u"fenix_edits.json"_q);
	if (!QFile::exists(deletedPath) && !QFile::exists(editsPath)) {
		return;
	}

	auto sessions = std::vector<not_null<Main::Session*>>{ current };
	for (const auto &[index, account] : Core::App().domain().accounts()) {
		if (const auto session = account->maybeSession()) {
			if (session != current) {
				sessions.push_back(session);
			}
		}
	}

	// Message ids of private chats and basic groups are counted per account
	// and the old files didn't say whose a record was: with more than one
	// account only the channels' records are certain, the rest is dropped.
	const auto all = (sessions.size() == 1);
	const auto keep = [&](FullMsgId key) {
		return key && (all || peerIsChannel(key.peer));
	};

	auto deleted = std::vector<std::pair<FullMsgId, DeletedBy>>();
	for (const auto &value : ReadLegacy(deletedPath)) {
		const auto object = value.toObject();
		const auto key = LegacyKey(object);
		const auto by = object.value(u"by"_q).toInt(0);
		if (keep(key)
			&& by >= int(DeletedBy::Other)
			&& by <= int(DeletedBy::Channel)) {
			deleted.emplace_back(key, DeletedBy(by));
		}
	}

	using Versions = std::vector<EditHistory::Version>;
	auto edits = std::vector<std::pair<FullMsgId, Versions>>();
	for (const auto &value : ReadLegacy(editsPath)) {
		const auto object = value.toObject();
		const auto key = LegacyKey(object);
		if (!keep(key)) {
			continue;
		}
		auto versions = Versions();
		for (const auto &entry : object.value(u"versions"_q).toArray()) {
			const auto version = entry.toObject();
			versions.push_back({
				version.value(u"text"_q).toString(),
				TimeId(version.value(u"date"_q).toVariant().toLongLong()),
			});
		}
		if (!versions.empty()) {
			edits.emplace_back(key, std::move(versions));
		}
	}

	for (const auto &session : sessions) {
		auto &stores = Resolve(session);
		stores.deleted.importLegacy(deleted);
		stores.edits.importLegacy(edits);
	}
	QFile::remove(deletedPath);
	QFile::remove(editsPath);
}

} // namespace

SessionFile::SessionFile(
	not_null<Main::Session*> session,
	const QString &name)
: _basePath(SessionBasePath(session))
, _name(name)
// The account's own local key, the one its other files are encrypted with.
, _key(session->local().peekLegacyLocalKey())
, _owner(session->uniqueId()) {
}

QByteArray SessionFile::read() const {
	if (!_key) {
		return QByteArray();
	}
	Storage::details::FileReadDescriptor file;
	if (!Storage::details::ReadEncryptedFile(file, _name, _basePath, _key)) {
		return QByteArray();
	}
	auto format = quint32();
	auto owner = quint64();
	auto payload = QByteArray();
	file.stream >> format >> owner >> payload;
	if (!Storage::details::CheckStreamStatus(file.stream)
		|| format != kFormat
		|| owner != _owner) {
		return QByteArray();
	}
	return payload;
}

void SessionFile::write(const QByteArray &payload) const {
	if (!_key) {
		return;
	}
	Storage::details::EncryptedDescriptor data(
		sizeof(quint32)
		+ sizeof(quint64)
		+ Serialize::bytearraySize(payload));
	data.stream << kFormat << quint64(_owner) << payload;

	Storage::details::FileWriteDescriptor file(_name, _basePath);
	file.writeEncrypted(data, _key);
}

void SessionFile::remove() const {
	for (const auto postfix : { 's', '0', '1' }) {
		QFile::remove(_basePath + _name + QChar(postfix));
	}
}

void RemoveSessionFiles(
		not_null<Main::Session*> session,
		const QString &prefix) {
	// A write still queued would bring a file back after its removal.
	Storage::details::Sync();
	const auto base = SessionBasePath(session);
	const auto names = QDir(base).entryList(
		{ prefix + u"*"_q },
		QDir::Files);
	for (const auto &name : names) {
		QFile::remove(base + name);
	}
}

DeletedMsg &GetDeleted(not_null<Main::Session*> session) {
	return Resolve(session).deleted;
}

EditHistory &GetEdits(not_null<Main::Session*> session) {
	return Resolve(session).edits;
}

KeptMessages &GetKept(not_null<Main::Session*> session) {
	return Resolve(session).kept;
}

void RememberOwnDeletion(
		not_null<History*> history,
		const QVector<MTPint> &ids) {
	auto &stores = Resolve(&history->session());
	const auto peerId = history->peer->id;
	stores.deleted.markOwnDeletion(peerId, ids);
	for (const auto &id : ids) {
		stores.edits.forget(peerId, MsgId(id.v));
		stores.kept.forget(peerId, MsgId(id.v));
	}
}

void ForgetSavedChat(not_null<History*> history) {
	auto &stores = Resolve(&history->session());
	const auto peerId = history->peer->id;
	stores.deleted.forgetChat(peerId);
	stores.edits.forgetChat(peerId);
	stores.kept.forgetChat(peerId);
}

void ForgetSavedBetween(
		not_null<History*> history,
		TimeId minDate,
		TimeId maxDate) {
	auto &stores = Resolve(&history->session());
	const auto peerId = history->peer->id;
	const auto ids = stores.kept.forgetBetween(peerId, minDate, maxDate);
	for (const auto id : ids) {
		stores.deleted.forget(peerId, id);
	}
}

namespace {

// The kept messages are gone from the server already: their items go from
// the chat now too, as they would have without Save Deleted Messages.
void DestroyKeptItems(
		not_null<History*> history,
		const std::vector<MsgId> &ids) {
	auto &data = history->owner();

	// An id can come both from the copies and the marks: each item once.
	auto unique = base::flat_set<not_null<HistoryItem*>>();
	for (const auto id : ids) {
		if (const auto item = data.message(history->peer, id)) {
			unique.emplace(item);
		}
	}
	if (unique.empty()) {
		return;
	}
	const auto items = std::vector<not_null<HistoryItem*>>(
		begin(unique),
		end(unique));
	data.notifyItemsAboutToBeDestroyed(items);
	for (const auto &item : items) {
		item->destroy();
	}
	if (!history->chatListMessageKnown()) {
		history->requestChatListMessage();
	}
}

} // namespace

void ForgetKeptMessage(not_null<History*> history, MsgId msgId) {
	auto &stores = Resolve(&history->session());
	const auto peerId = history->peer->id;
	stores.kept.forget(peerId, msgId);
	stores.deleted.forget(peerId, msgId);
	DestroyKeptItems(history, { msgId });
}

void ForgetKeptChatMessages(not_null<History*> history) {
	auto &stores = Resolve(&history->session());
	const auto peerId = history->peer->id;
	auto ids = std::vector<MsgId>();
	for (const auto &kept : stores.kept.list(peerId)) {
		ids.push_back(kept.id);
	}
	for (const auto &id : stores.deleted.all()) {
		if (id.peer == peerId) {
			ids.push_back(id.msg);
		}
	}
	stores.kept.forgetChat(peerId);
	stores.deleted.forgetChat(peerId);
	DestroyKeptItems(history, ids);
}

void ClearSavedHistory(not_null<Main::Session*> session) {
	auto &stores = Resolve(session);
	const auto kept = stores.deleted.all();
	stores.deleted.clear();
	stores.edits.clear();
	stores.kept.clear();

	// The kept messages are gone from the server already: now they go from
	// the chats too, as they would have without Save Deleted Messages.
	auto &data = session->data();
	auto items = std::vector<not_null<HistoryItem*>>();
	auto histories = base::flat_set<not_null<History*>>();
	for (const auto &id : kept) {
		if (const auto item = data.message(id)) {
			items.push_back(item);
			histories.emplace(item->history());
		}
	}
	if (!items.empty()) {
		data.notifyItemsAboutToBeDestroyed(items);
		for (const auto &item : items) {
			item->destroy();
		}
	}
	for (const auto &history : histories) {
		if (!history->chatListMessageKnown()) {
			history->requestChatListMessage();
		}
	}
}

void WipeSavedHistory(not_null<Main::Session*> session) {
	if (const auto stores = Find(session)) {
		stores->deleted.wipe();
		stores->edits.wipe();
		stores->kept.wipe();
	}

	// A write still queued would bring a file back after its removal.
	Storage::details::Sync();
	SessionFile(session, QString::fromLatin1(kDeletedFileName)).remove();
	SessionFile(session, QString::fromLatin1(kEditsFileName)).remove();
	RemoveSessionFiles(session, QString::fromLatin1(kKeptFilePrefix));
}

} // namespace Fenix
