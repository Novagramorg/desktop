/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Boxes for the per-chat lock (see fenix/chat_lock):
 - ChatLockUnlockBox: asks for the passcode when a locked chat is opened.
 - ChatLockChoiceBox: "Common password" vs "Separate password" picker shown when
   locking a chat for the first time.
 - ChatLockCreateBox: sets a new passcode (enter + confirm) for the chat.
*/
#pragma once

#include "base/object_ptr.h"
#include "data/data_peer_id.h"

namespace Ui {
class GenericBox;
} // namespace Ui

namespace Fenix {

// Verify the passcode to reveal a locked chat. Calls [onUnlocked] on success
// (the chat is also marked unlocked for the rest of the session).
void ChatLockUnlockBox(
	not_null<Ui::GenericBox*> box,
	PeerId peerId,
	Fn<void()> onUnlocked);

// Pick how to lock a chat: [onChoose](true) for a separate per-chat passcode,
// [onChoose](false) for the shared common passcode.
void ChatLockChoiceBox(
	not_null<Ui::GenericBox*> box,
	Fn<void(bool individual)> onChoose);

// Create a passcode (enter + confirm) and lock the chat with it. [individual]
// selects the per-chat bucket; otherwise the common passcode is (re)set.
// Calls [onDone] after the lock is saved.
void ChatLockCreateBox(
	not_null<Ui::GenericBox*> box,
	PeerId peerId,
	bool individual,
	Fn<void()> onDone);

// Change the shared/common passcode itself (not tied to locking a chat). Every
// chat on the common passcode immediately uses the new one. [onDone] runs after.
void ChatLockCommonPasswordBox(
	not_null<Ui::GenericBox*> box,
	Fn<void()> onDone);

} // namespace Fenix
