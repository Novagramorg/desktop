/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Unread message reminder. Port of the Android org.fenixuz.utils.MessageReminder
"budilnik": while enabled, if an incoming PRIVATE message stays unread for the
chosen delay, the message's notification is re-surfaced once (native popup +
sound). Reading the chat before the delay cancels it.

Desktop differences from the Android alarm version:
 - No AlarmManager: a base::Timer drives the single fire, so the reminder works
   while the app is running (including minimised to tray). A fully closed app
   can't fire — acceptable for a desktop reminder.
 - Re-uses the normal notification pipeline (Window::Notifications), which is
   automatically skipped when the chat is muted or currently focused — exactly
   the cases where a reminder is unwanted — so no extra checks are needed.

One reminder is outstanding at a time (re-arms on the next incoming message,
not per message). Owned per-account by Data::Histories, so the timer and the
target peer never outlive their session.
*/
#pragma once

#include "base/timer.h"
#include "data/data_peer_id.h"

class HistoryItem;

namespace Data {
class Session;
} // namespace Data

namespace Fenix {

class Reminder final {
public:
	explicit Reminder(not_null<Data::Session*> owner);

	// Fed from Data::Session::newItemAdded for each freshly-added message.
	void process(not_null<HistoryItem*> item);

private:
	void fire();

	const not_null<Data::Session*> _owner;
	base::Timer _timer;
	PeerId _targetPeer = PeerId();
	bool _armed = false;
};

} // namespace Fenix
