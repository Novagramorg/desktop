/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Per-chat lock. A port of the Android org.fenixuz.utils.Password store, with the
same two-mode model:
 - COMMON:     one shared passcode protects a set of chats (adding another chat
               needs no new passcode).
 - INDIVIDUAL: a chat carries its OWN passcode, independent of every other chat.

A chat is locked if it sits in either bucket. When a locked chat is opened it is
gated behind a passcode box (see fenix/chat_lock_box) before its history shows;
the box verifies against the chat's individual passcode if it has one, otherwise
against the common passcode.

Passcodes are stored hashed (SHA-256), never in clear text. The locked sets and
the common passcode persist on-device in tdata/fenix_chatlock.json (survive
logout, no network). The "already unlocked this session" set is transient and
lives only in memory — it clears on restart, re-prompting next launch.
*/
#pragma once

#include "data/data_peer_id.h"

#include "base/flat_set.h"
#include "base/flat_map.h"

namespace Fenix {

class ChatLock final {
public:
	[[nodiscard]] static ChatLock &Instance();

	// Queries (persisted state).
	[[nodiscard]] bool isLocked(PeerId peerId) const;
	[[nodiscard]] bool isIndividual(PeerId peerId) const;
	[[nodiscard]] bool hasCommonPassword() const;
	[[nodiscard]] bool hasAnyLock() const;

	// Verify [input] against the chat's effective passcode (individual if set,
	// otherwise the common one). False when the chat has no passcode at all.
	[[nodiscard]] bool check(PeerId peerId, const QString &input) const;

	// Verify [input] against the common passcode alone. False when there is
	// no common passcode.
	[[nodiscard]] bool checkCommon(const QString &input) const;

	// Mutations.
	void changeCommonPassword(const QString &passcode);
	void addCommonLock(PeerId peerId);
	void addIndividualLock(PeerId peerId, const QString &passcode);
	void unlock(PeerId peerId);   // remove the lock, whichever bucket it was in
	void removeEverything();      // drop the common passcode and unlock all

	// Transient session-unlocked set (not persisted). A chat stays unlocked only
	// while it is the one you're viewing: markUnlocked() when its passcode is
	// passed, and relockAllExcept() re-locks every other chat as soon as the
	// active chat changes — so re-opening a locked chat always asks again, like
	// the Android fork (where the unlock is per chat-open, not per app session).
	[[nodiscard]] bool isUnlocked(PeerId peerId) const;
	void markUnlocked(PeerId peerId);
	void relockAllExcept(PeerId keep);

private:
	ChatLock();
	~ChatLock() = default;

	void load();
	void save();

	[[nodiscard]] QString effectiveHash(PeerId peerId) const;

	QString _commonHash;                       // empty = no common passcode
	base::flat_set<PeerId> _commonIds;         // locked with the common passcode
	base::flat_map<PeerId, QString> _individual; // peer -> its own passcode hash
	base::flat_set<PeerId> _unlocked;          // transient: passed this session
};

[[nodiscard]] inline ChatLock &GetChatLock() {
	return ChatLock::Instance();
}

} // namespace Fenix
