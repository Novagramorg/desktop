/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/message_cache.h"

#include "base/unixtime.h"
#include "base/weak_ptr.h"
#include "core/application.h"
#include "data/data_session.h"
#include "fenix/deleted_msg.h"
#include "fenix/edit_history.h"
#include "fenix/fenix_settings.h"
#include "fenix/kept_messages.h"
#include "history/history.h"
#include "main/main_session.h"
#include "storage/storage_account.h"
#include "storage/storage_encryption.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

namespace Fenix {
namespace {

// Others' messages sent in the last month: the ones a delete or an edit is
// still likely to come for. Older ones loaded from the history are not
// stored, so scrolling back through a chat doesn't fill the cache.
constexpr auto kMaxAge = TimeId(30 * 24 * 60 * 60);

// More than a hundred thousand messages. The ones looked at longest ago go
// first, and any not looked at for a month.
constexpr auto kTotalSizeLimit = int64(64 * 1024 * 1024);
constexpr auto kTotalTimeLimit = 31 * 24 * 60 * 60;

// A message with more data than this is not stored, as in kept_messages.
constexpr auto kMaxDataSize = 256 * 1024;

[[nodiscard]] QString DatabasePath(not_null<Main::Session*> session) {
	// Next to Telegram's cache, apart from it: Clear cache leaves it alone.
	return QFileInfo(session->local().cachePath()).absolutePath()
		+ u"/novagram_messages"_q;
}

[[nodiscard]] Storage::Cache::Database::Settings DatabaseSettings() {
	auto result = Storage::Cache::Database::Settings();
	result.clearOnWrongKey = true;
	result.totalSizeLimit = kTotalSizeLimit;
	result.totalTimeLimit = kTotalTimeLimit;
	result.maxDataSize = kMaxDataSize;
	return result;
}

[[nodiscard]] Storage::Cache::Key KeyFor(PeerId peerId, MsgId msgId) {
	// Private chats and groups count message ids for the whole account.
	return {
		peerIsChannel(peerId) ? SerializePeerId(peerId) : uint64(0),
		uint64(msgId.bare),
	};
}

[[nodiscard]] bool Active() {
	return Get().saveDeletedMsg() || Get().saveEditHistory();
}

// The text the message had before an edit Novagram didn't see it get.
void RecordCachedEdit(
		not_null<Main::Session*> session,
		PeerId peerId,
		MsgId msgId,
		const QByteArray &was,
		const QString &now) {
	if (!Get().saveEditHistory()) {
		return;
	}
	const auto message = ParseKept(was);
	if (!message) {
		return;
	}
	message->match([&](const MTPDmessage &data) {
		if (peerFromMTP(data.vpeer_id()) != peerId
			|| MsgId(data.vid().v) != msgId) {
			return;
		}
		const auto text = qs(data.vmessage());
		if (text != now) {
			const auto edited = data.vedit_date();
			GetEdits(session).recordEdit(
				peerId,
				msgId,
				text,
				edited ? edited->v : data.vdate().v);
		}
	}, [](const auto &) {
	});
}

// A message deleted while Novagram didn't have it loaded.
void KeepCachedDeleted(
		not_null<Main::Session*> session,
		PeerId channelId,
		MsgId msgId,
		const QByteArray &data) {
	if (!Get().saveDeletedMsg()) {
		return;
	}
	const auto message = ParseKept(data);
	if (!message) {
		return;
	}
	const auto id = message->match([](const MTPDmessage &data) {
		return FullMsgId(peerFromMTP(data.vpeer_id()), data.vid().v);
	}, [](const auto &) {
		return FullMsgId();
	});
	if (!id
		|| (id.msg != msgId)
		|| (channelId && id.peer != channelId)
		|| session->data().message(id)) {
		return;
	}
	auto &deleted = GetDeleted(session);
	if (deleted.takeOwnDeletion(id.peer, id.msg)
		|| !GetKept(session).keepCopy(id.peer, id.msg, data)) {
		return;
	}
	deleted.recordDeletion(
		id.peer,
		id.msg,
		peerIsChannel(id.peer) ? DeletedBy::Channel : DeletedBy::Other);
	if (const auto history = session->data().historyLoaded(id.peer)) {
		KeptHistory::ShowLate(history, id.msg);
	}
}

} // namespace

MessageCache::MessageCache(not_null<Main::Session*> session)
: _session(session) {
	// With both off nothing is kept, so the copies go too.
	rpl::combine(
		Get().saveDeletedMsgValue(),
		Get().saveEditHistoryValue(),
		[](bool deleted, bool edits) { return deleted || edits; }
	) | rpl::distinct_until_changed(
	) | rpl::skip(1) | rpl::filter([](bool active) {
		return !active;
	}) | rpl::to_empty | rpl::on_next([=] {
		clear();
	}, _lifetime);
}

MessageCache::~MessageCache() = default;

Storage::Cache::Database &MessageCache::database() {
	if (!_database) {
		_database.emplace(Core::App().databases().get(
			DatabasePath(_session),
			DatabaseSettings()));
		(*_database)->open(_session->local().cacheKey());
	}
	return **_database;
}

Storage::Cache::Database *MessageCache::activeDatabase() {
	return (_wiped || !Active()) ? nullptr : &database();
}

void MessageCache::remember(const MTPMessage &message) {
	message.match([&](const MTPDmessage &data) {
		const auto peerId = peerFromMTP(data.vpeer_id());
		const auto msgId = MsgId(data.vid().v);
		if (data.is_out()
			|| !IsServerMsgId(msgId)
			|| (data.vdate().v < base::unixtime::now() - kMaxAge)) {
			return;
		}
		const auto database = activeDatabase();
		if (!database
			|| GetDeleted(_session).whoDeleted(peerId, msgId).has_value()) {
			return; // A kept one has its copy already.
		}
		auto bytes = SerializeMessage(message);
		if (bytes.size() <= kMaxDataSize) {
			database->putIfEmpty(KeyFor(peerId, msgId), std::move(bytes));
		}
	}, [](const auto &) {
	});
}

void MessageCache::edited(const MTPMessage &message, bool loaded) {
	message.match([&](const MTPDmessage &data) {
		const auto peerId = peerFromMTP(data.vpeer_id());
		const auto msgId = MsgId(data.vid().v);
		if (data.is_out() || !IsServerMsgId(msgId)) {
			return;
		}
		const auto database = activeDatabase();
		if (!database) {
			return;
		}
		const auto key = KeyFor(peerId, msgId);
		if (!loaded && Get().saveEditHistory()) {
			// A loaded message saves its edit itself, see applyEdition.
			// The database runs in order: this reads the copy before the
			// put below replaces it.
			// The answer comes on the database's thread.
			const auto session = _session;
			const auto weak = base::make_weak(session.get());
			const auto now = qs(data.vmessage());
			database->get(key, [=](QByteArray &&was) {
				if (was.isEmpty()) {
					return;
				}
				crl::on_main(weak, [=, copy = std::move(was)] {
					RecordCachedEdit(session, peerId, msgId, copy, now);
				});
			});
		}
		auto bytes = SerializeMessage(message);
		if (bytes.size() <= kMaxDataSize) {
			database->put(key, std::move(bytes));
		} else {
			database->remove(key);
		}
	}, [](const auto &) {
	});
}

void MessageCache::deleted(PeerId channelId, MsgId msgId) {
	if (!IsServerMsgId(msgId) || !Get().saveDeletedMsg()) {
		return;
	}
	const auto database = activeDatabase();
	if (!database) {
		return;
	}
	const auto key = KeyFor(channelId, msgId);

	// The answer comes on the database's thread.
	const auto session = _session;
	const auto weak = base::make_weak(session.get());
	database->get(key, [=](QByteArray &&data) {
		if (data.isEmpty()) {
			return;
		}
		crl::on_main(weak, [=, copy = std::move(data)] {
			KeepCachedDeleted(session, channelId, msgId, copy);
		});
	});

	// Its copy goes to the chat's kept messages, or it isn't kept.
	database->remove(key);
}

void MessageCache::forget(PeerId peerId, MsgId msgId) {
	if (_database && !_wiped) {
		(*_database)->remove(KeyFor(peerId, msgId));
	}
}

void MessageCache::clear() {
	if (_wiped
		|| (!_database && !QDir(DatabasePath(_session)).exists())) {
		return;
	}
	database().clear();
}

void MessageCache::wipe() {
	if (_wiped) {
		return;
	} else if (!_database && !QDir(DatabasePath(_session)).exists()) {
		_wiped = true;
		return;
	}
	auto &database = this->database();
	_wiped = true;

	// As Telegram does with its own cache when the account logs out.
	database.close();
	database.clear();
}

void RemoveMessageCacheFiles(not_null<Main::Session*> session) {
	QDir(DatabasePath(session)).removeRecursively();
}

rpl::producer<int64> MessageCache::sizeValue() const {
	if (!_database) {
		return rpl::single(int64(0));
	}
	return (*_database)->statsOnMain(
	) | rpl::map([](const Storage::Cache::Database::Stats &stats) {
		return stats.full.totalSize;
	});
}

} // namespace Fenix
