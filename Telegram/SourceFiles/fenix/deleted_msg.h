/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Tracks messages deleted by others so the local view can keep them visible
with a "deleted by" annotation. Equivalent of Android org.fenixuz.utils
DeletedMsg. One store per account, see fenix/saved_history.h.
*/
#pragma once

#include "base/timer.h"
#include "fenix/saved_history.h"

class HistoryItem;

namespace Fenix {

enum class DeletedBy {
	Other = 0,
	Me = 1,
	Channel = 2,
};

class DeletedMsg final {
public:
	explicit DeletedMsg(not_null<Main::Session*> session);
	~DeletedMsg();

	void recordDeletion(PeerId peerId, MsgId msgId, DeletedBy by);
	[[nodiscard]] std::optional<DeletedBy> whoDeleted(
		PeerId peerId,
		MsgId msgId) const;
	[[nodiscard]] std::vector<FullMsgId> all() const;
	void forget(PeerId peerId, MsgId msgId);
	void forgetChat(PeerId peerId);
	void clear();

	// The user is deleting these messages: when the server reports them
	// deleted, they go away instead of being kept.
	void markOwnDeletion(PeerId peerId, const QVector<MTPint> &ids);
	[[nodiscard]] bool takeOwnDeletion(PeerId peerId, MsgId msgId);

	// Marks of the old file shared by all accounts, see saved_history.cpp.
	void importLegacy(const std::vector<std::pair<FullMsgId, DeletedBy>> &list);

	// The account logs out: nothing is kept or written any more.
	void wipe();

private:
	struct Record {
		DeletedBy by = DeletedBy::Other;
		TimeId at = 0;
	};

	void load();
	void save();
	void saveDelayed();
	void trim();

	const SessionFile _file;
	std::map<FullMsgId, Record> _records;
	base::flat_map<FullMsgId, crl::time> _own;
	base::Timer _saveTimer;
	bool _wiped = false;

};

[[nodiscard]] DeletedMsg &GetDeleted(not_null<Main::Session*> session);

// Whether a message the server reports deleted stays in the chat: not when
// the user deleted it from this app, its auto-delete timer ran out, or it
// is a service message, which has no place for the "deleted" mark.
[[nodiscard]] bool KeepDeleted(not_null<HistoryItem*> item);

} // namespace Fenix
