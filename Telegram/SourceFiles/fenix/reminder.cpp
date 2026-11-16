/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/reminder.h"

#include "fenix/fenix_settings.h"
#include "base/bytes.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "data/data_changes.h"
#include "data/data_histories.h"
#include "data/data_session.h"
#include "data/data_peer.h"
#include "data/data_thread.h"
#include "data/notify/data_notify_settings.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "media/audio/media_audio.h"
#include "media/audio/media_audio_track.h"
#include "window/notifications_manager.h"

#include <QtCore/QFile>

namespace Fenix {
namespace {

// As long as the Android reminder rings, so it is hard to miss.
constexpr auto kRingDuration = crl::time(8000);

// The system's alarm sound, as Android's alarm tone. Without one, Telegram's
// ringtone, the loudest sound it has.
[[nodiscard]] QString AlarmSoundPath() {
#ifdef Q_OS_WIN
	const auto root = qEnvironmentVariable("SystemRoot");
	if (!root.isEmpty()) {
		const auto path = root + u"/Media/Alarm01.wav"_q;
		if (QFile::exists(path)) {
			return path;
		}
	}
#endif // Q_OS_WIN
	return Core::App().settings().getSoundPath(u"call_incoming"_q);
}

// The notification tone is the chat's notification sound, or the default one
// when the chat has none of its own.
[[nodiscard]] QByteArray SoundBytes(
		not_null<Data::Session*> owner,
		History *history,
		ReminderSound sound) {
	if (sound == ReminderSound::Alarm) {
		auto file = QFile(AlarmSoundPath());
		return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
	}
	auto &notifications = Core::App().notifications();
	const auto id = history
		? owner->notifySettings().sound(history).id
		: DocumentId();
	const auto bytes = id
		? notifications.lookupSoundBytes(owner, id)
		: QByteArray();
	return bytes.isEmpty()
		? notifications.lookupSoundBytes(owner, DocumentId())
		: bytes;
}

[[nodiscard]] std::unique_ptr<Media::Audio::Track> MakeTrack(
		const QByteArray &bytes) {
	if (bytes.isEmpty()) {
		return nullptr;
	}
	auto result = Media::Audio::Current().createTrack();
	result->fillFromData(bytes::make_vector(bytes));
	return result->failed() ? nullptr : std::move(result);
}

// Notifications of this account show at all: the others' only when the
// user wants notifications from all accounts.
[[nodiscard]] bool AccountNotifies(not_null<Data::Session*> owner) {
	return Core::App().settings().notifyFromAll()
		|| (&owner->session().account() == &Core::App().domain().active());
}

} // namespace

Reminder::Reminder(not_null<Data::Session*> owner)
: _owner(owner)
, _timer([=] { fire(); })
, _ringTimer([=] { stopRing(); }) {
	Get().messageReminderChanges(
	) | rpl::filter([](bool on) {
		return !on;
	}) | rpl::on_next([=] {
		disarm();
	}, _lifetime);
}

Reminder::~Reminder() = default;

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
	// A muted chat's messages don't notify, so they don't remind either.
	if (_owner->notifySettings().isMuted(history)) {
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
	_watchedItem = item->fullId();
	_armed = true;
	watchRead(history);
	_timer.callOnce(crl::time(Get().messageReminderDelay()) * 60 * 1000);
}

void Reminder::watchRead(not_null<History*> history) {
	// Not destroyed from its own handler: the next arm replaces it.
	_readLifetime.destroy();
	_owner->session().changes().historyUpdates(
		history,
		Data::HistoryUpdate::Flag::UnreadView
	) | rpl::on_next([=] {
		if (!_armed && !_ring) {
			return;
		}
		// Read by the user: the reminder is off, and a ring stops.
		const auto item = _owner->message(_watchedItem);
		if (!item || !item->unread(history)) {
			disarm();
		}
	}, _readLifetime);
}

void Reminder::disarm() {
	_armed = false;
	_timer.cancel();
	stopRing();
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
	if (_owner->notifySettings().isMuted(history)
		|| !AccountNotifies(_owner)) {
		return;
	}
	_watchedItem = item->fullId();
	if (ring(history)) {
		_silentItem = item->fullId();
	}
	// Show its notification again through the normal pipeline, silent as the
	// reminder rings itself. It's skipped if the chat is currently focused.
	Core::App().notifications().schedule(Data::ItemNotification{
		.item = item,
	});
}

bool Reminder::ring(not_null<History*> history) {
	stopRing();
	const auto sound = Get().messageReminderSound();
	auto track = MakeTrack(SoundBytes(_owner, history, sound));
	if (!track) {
		// Nothing to ring with: the notification keeps its own sound.
		return false;
	}
	const auto owned = std::make_shared<std::unique_ptr<Media::Audio::Track>>(
		std::move(track));
	const auto play = crl::guard(this, [=] {
		if (!*owned) {
			return;
		}
		_ring = std::move(*owned);
		_ring->playInLoop();
		Media::Player::mixer()->suppressAll(kRingDuration);
		Media::Player::mixer()->scheduleFaderCallback();
		_ringTimer.callOnce(kRingDuration);
	});
	if (sound == ReminderSound::Alarm) {
		// An alarm rings as on Android: also when notification sounds are
		// off or the system is set not to disturb.
		play();
	} else {
		// A notification tone follows the notification sound settings, as
		// the notification's own sound would.
		Core::App().notifications().manager().maybePlaySound(play);
	}
	return true;
}

void Reminder::stopRing() {
	_ringTimer.cancel();
	_ring = nullptr;
}

bool Reminder::silences(not_null<const HistoryItem*> item) const {
	return (item->fullId() == _silentItem);
}

bool ReminderSilences(not_null<const HistoryItem*> item) {
	return item->history()->owner().histories().reminder().silences(item);
}

std::unique_ptr<Media::Audio::Track> PreviewReminderSound(
		not_null<Data::Session*> owner,
		ReminderSound sound) {
	auto result = MakeTrack(SoundBytes(owner, nullptr, sound));
	if (result) {
		result->playOnce();
	}
	return result;
}

} // namespace Fenix
