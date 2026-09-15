/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/reminder.h"

#include "fenix/fenix_settings.h"
#include "core/application.h"
#include "data/data_session.h"
#include "data/data_peer.h"
#include "data/data_thread.h"
#include "history/history.h"
#include "history/history_item.h"
#include "window/notifications_manager.h"

namespace Fenix {

Reminder::Reminder(not_null<Data::Session*> owner)
: _owner(owner)
, _timer([=] { fire(); }) {
}

void Reminder::process(not_null<HistoryItem*> item) {
	if (!Get().messageReminder() || _armed) {
		return;
	}
	if (item->out()) {
		return;
	}
	const auto history = item->history();
	// Private chats only — matches the Android original; avoids nagging on
	// busy groups/channels.
	if (!history->peer->isUser()) {
		return;
	}
	// Already read (e.g. the chat is open and focused) — nothing to remind.
	// NB: use item->unread() rather than history->unreadCount(), because
	// process() runs from notifyNewItemAdded() which fires *before* the
	// unread counter is incremented for this very message — the counter would
	// still read 0 for a previously-read chat and the reminder would never arm.
	if (!item->unread(history)) {
		return;
	}
	_targetPeer = history->peer->id;
	_armed = true;
	_timer.callOnce(crl::time(Get().messageReminderDelay()) * 60 * 1000);
}

void Reminder::fire() {
	_armed = false;
	if (!Get().messageReminder() || !_targetPeer) {
		return;
	}
	const auto history = _owner->historyLoaded(_targetPeer);
	// Read in the meantime — nothing to remind about.
	if (!history || history->unreadCount() <= 0) {
		return;
	}
	const auto item = history->lastMessage();
	if (!item || item->out()) {
		return;
	}
	// Re-surface through the normal notification pipeline: native popup + sound,
	// automatically skipped if the chat is muted or currently focused.
	Core::App().notifications().schedule(Data::ItemNotification{
		.item = item,
	});
}

} // namespace Fenix
