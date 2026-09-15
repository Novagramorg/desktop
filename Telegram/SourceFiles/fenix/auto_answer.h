/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Auto-answer: when enabled, replies once to the first incoming message of each
private chat with a saved text. The "answered" set is per-session and re-armed
when you open the chat, so it acts like an away-message. Equivalent of the
Android org.fenixuz.ui.auto_answer.AutoAnswer engine. The on/off flag and the
reply text live in Fenix::Settings; this engine only holds the runtime logic.
*/
#pragma once

#include "data/data_peer_id.h"
#include "base/flat_set.h"

class HistoryItem;

namespace Fenix {

class AutoAnswer final {
public:
	[[nodiscard]] static AutoAnswer &Instance();

	// Called for every freshly-added message (see Data::Session::newItemAdded).
	void process(not_null<HistoryItem*> item);

	// Re-arm a chat so it can be auto-answered again (called when you open it).
	void markOpened(PeerId peerId);

private:
	AutoAnswer() = default;
	~AutoAnswer() = default;

	base::flat_set<PeerId> _answered;
};

[[nodiscard]] inline AutoAnswer &GetAutoAnswer() {
	return AutoAnswer::Instance();
}

} // namespace Fenix
