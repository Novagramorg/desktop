/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Full copies of messages deleted by others (Android HistoryDb): the server's
own data of each message, kept per chat in the account's encrypted files.
A kept message looks as it did and takes its place in the chat again after
a restart or a reload of the history. See fenix/saved_history.h.
*/
#pragma once

#include "base/timer.h"
#include "fenix/saved_history.h"

#include <map>
#include <vector>

class History;
class HistoryItem;

namespace Fenix {

class KeptMessages final {
public:
	struct Kept {
		MsgId id = 0;
		TimeId at = 0; // When it was kept.
		QByteArray data; // The message, serialized MTPMessage.
	};

	explicit KeptMessages(not_null<Main::Session*> session);
	~KeptMessages();

	// While Save Deleted Messages is on, the server data of the loaded
	// messages is held in memory, ready to be kept if someone deletes them.
	void remember(const MTPMessage &message);

	// Someone deleted this message: its copy goes to the chat's file.
	void keep(not_null<HistoryItem*> item);

	// The chat's kept messages, by id. Reads the chat's file on first use.
	[[nodiscard]] const std::vector<Kept> &list(PeerId peerId);
	void forget(PeerId peerId, MsgId msgId);
	void forgetChat(PeerId peerId);
	// The copies of the messages sent between the dates, not including
	// them, as the server deletes them. Returns the ids forgotten.
	std::vector<MsgId> forgetBetween(
		PeerId peerId,
		TimeId minDate,
		TimeId maxDate);
	void clear();

	// The account logs out: nothing is kept or written any more.
	void wipe();

	[[nodiscard]] rpl::producer<PeerId> changes() const;

private:
	struct Chat {
		std::vector<Kept> list;
		bool loaded = false;
	};

	[[nodiscard]] Chat &chat(PeerId peerId);
	[[nodiscard]] SessionFile file(PeerId peerId) const;
	void load(PeerId peerId, Chat &chat);
	void changed(PeerId peerId);
	void saveDirty();
	void save(PeerId peerId, const Chat &chat);

	const not_null<Main::Session*> _session;
	std::map<FullMsgId, QByteArray> _loaded;
	std::map<PeerId, Chat> _chats;
	base::flat_set<PeerId> _dirty;
	base::Timer _saveTimer;
	rpl::event_stream<PeerId> _changes;
	bool _wiped = false;
	rpl::lifetime _lifetime;

};

[[nodiscard]] KeptMessages &GetKept(not_null<Main::Session*> session);

[[nodiscard]] std::optional<MTPMessage> ParseKept(const QByteArray &data);

// Holds the server data of a message being loaded, see remember().
void RememberLoaded(
	not_null<Main::Session*> session,
	const MTPMessage &message);

// Works on History's blocks, as its friend.
class KeptHistory final {
public:
	// Puts the kept messages of the history back where they belong, in the
	// part of the history that is loaded. Runs after every loaded slice.
	static void Restore(not_null<History*> history);

	// The same for the chat view built on the history's ids (see
	// Data::HistoryMessages): adds the kept messages inside a slice's
	// range to its ids, before the slice is added, and counts them all in
	// the chat's total, so the view still asks for the rest of the chat.
	static void AddToSlice(
		not_null<History*> history,
		std::vector<MsgId> &ids,
		MsgRange range,
		int *fullCount = nullptr);

	// The item of a kept message, created from its copy when needed (it is
	// not added to the history's blocks).
	[[nodiscard]] static HistoryItem *Item(
		not_null<History*> history,
		const KeptMessages::Kept &kept);

private:
	static void Insert(
		not_null<History*> history,
		not_null<HistoryItem*> item);

};

} // namespace Fenix
