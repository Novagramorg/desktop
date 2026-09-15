/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/auto_answer.h"

#include "fenix/fenix_settings.h"
#include "api/api_common.h"
#include "apiwrap.h"
#include "data/data_user.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"

namespace Fenix {

AutoAnswer &AutoAnswer::Instance() {
	static AutoAnswer instance;
	return instance;
}

void AutoAnswer::process(not_null<HistoryItem*> item) {
	if (!Get().autoAnswer()) {
		return;
	}
	// Only reply to genuine incoming messages from a real user (not yourself,
	// not a bot, not service/local), and only once per chat until re-armed.
	if (item->out() || item->isService() || !item->isRegular()) {
		return;
	}
	const auto history = item->history();
	const auto user = history->peer->asUser();
	if (!user || user->isSelf() || user->isBot()) {
		return;
	}
	const auto text = Get().autoAnswerText().trimmed();
	if (text.isEmpty()) {
		return;
	}
	if (_answered.contains(history->peer->id)) {
		return; // Already answered this chat.
	}
	_answered.emplace(history->peer->id);
	auto message = Api::MessageToSend(Api::SendAction(history));
	message.textWithTags = { text, {} };
	history->session().api().sendMessage(std::move(message));
}

void AutoAnswer::markOpened(PeerId peerId) {
	_answered.remove(peerId);
}

} // namespace Fenix
