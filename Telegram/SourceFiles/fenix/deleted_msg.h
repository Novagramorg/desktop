/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.

Tracks messages deleted by others so the local view can keep them visible
with a "deleted by" annotation. Equivalent of Android org.pro.utils.DeletedMsg.
Storage: JSON file at cWorkingDir()/tdata/fenix_deleted.json
*/
#pragma once

#include <rpl/event_stream.h>

namespace Fenix {

enum class DeletedBy {
	Other = 0,
	Me = 1,
	Channel = 2,
};

class DeletedMsg final {
public:
	[[nodiscard]] static DeletedMsg &Instance();

	void recordDeletion(PeerId peerId, MsgId msgId, DeletedBy by);
	[[nodiscard]] std::optional<DeletedBy> whoDeleted(
		PeerId peerId,
		MsgId msgId) const;
	void forget(PeerId peerId, MsgId msgId);
	void forgetAll(PeerId peerId);
	[[nodiscard]] int countFor(PeerId peerId) const;

	// Set to true just before the user issues their own delete request.
	// Cleared automatically after the next deletion update is processed.
	void markNextAsMine();
	[[nodiscard]] bool takeNextAsMine();

	[[nodiscard]] rpl::producer<> changes() const;

private:
	DeletedMsg();
	~DeletedMsg() = default;

	void load();
	void save();
	void scheduleSave();

	struct Key {
		PeerId peer;
		MsgId msg;
		friend constexpr bool operator<(
				const Key &a, const Key &b) noexcept {
			return std::tie(a.peer, a.msg) < std::tie(b.peer, b.msg);
		}
		friend constexpr bool operator==(
				const Key &a, const Key &b) noexcept = default;
	};
	std::map<Key, DeletedBy> _records;
	bool _markNextAsMine = false;
	rpl::event_stream<> _changes;
};

[[nodiscard]] inline DeletedMsg &GetDeleted() {
	return DeletedMsg::Instance();
}

} // namespace Fenix
