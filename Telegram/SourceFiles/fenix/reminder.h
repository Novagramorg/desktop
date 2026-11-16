/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Unread message reminder. Port of the Android org.fenixuz.utils.MessageReminder
"budilnik": while enabled, if an incoming PRIVATE message stays unread for the
chosen delay, the message's notification is shown once more, silent, and the
chosen sound rings for about 8 seconds: the chat's notification sound or the
system's alarm sound. Reading the chat cancels it, and stops a ring.

Desktop differences from the Android alarm version:
 - No AlarmManager: a base::Timer drives the single fire, so the reminder works
   while the app is running (including minimised to tray). A fully closed app
   can't fire — acceptable for a desktop reminder.
 - Re-uses the normal notification pipeline (Window::Notifications), which
   skips the notification when the chat is currently focused.
 - Muted chats don't arm it, as their messages don't notify on Android either.

One reminder is outstanding at a time (re-arms on the next incoming message,
not per message). Owned per-account by Data::Histories, so the timer and the
target peer never outlive their session.
*/
#pragma once

#include "base/timer.h"
#include "base/weak_ptr.h"
#include "data/data_msg_id.h"

class History;
class HistoryItem;

namespace Data {
class Session;
} // namespace Data

namespace Media::Audio {
class Track;
} // namespace Media::Audio

namespace Fenix {

enum class ReminderSound;

class Reminder final : public base::has_weak_ptr {
public:
	explicit Reminder(not_null<Data::Session*> owner);
	~Reminder();

	// Fed from Data::Session::newItemAdded for each freshly-added message.
	void process(not_null<HistoryItem*> item);

	// The notification the reminder shows again: it rings itself, so the
	// notification is silent.
	[[nodiscard]] bool silences(not_null<const HistoryItem*> item) const;

private:
	void watchRead(not_null<History*> history);
	void disarm();
	void fire();
	[[nodiscard]] bool ring(not_null<History*> history);
	void stopRing();

	const not_null<Data::Session*> _owner;
	base::Timer _timer;
	base::Timer _ringTimer;
	PeerId _targetPeer = PeerId();
	FullMsgId _watchedItem;
	FullMsgId _silentItem;
	std::unique_ptr<Media::Audio::Track> _ring;
	rpl::lifetime _readLifetime;
	rpl::lifetime _lifetime;
	bool _armed = false;
};

// Whether the reminder of the item's account shows this notification.
[[nodiscard]] bool ReminderSilences(not_null<const HistoryItem*> item);

// [sound] played once, as when it is chosen in the settings. It stops when
// the track is destroyed. Null when there is nothing to play.
[[nodiscard]] std::unique_ptr<Media::Audio::Track> PreviewReminderSound(
	not_null<Data::Session*> owner,
	ReminderSound sound);

} // namespace Fenix
