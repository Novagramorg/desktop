/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.

Runtime-only state that marks the chat currently opened in "ghost preview" mode
so the read-receipt paths can be suppressed for it — a preview must leave ZERO
footprint (no "read" blue checks, no "listened"/"watched" content receipt to the
other side). Equivalent of the Android org.fenixuz.utils.ChatPreviewState.

Scoped by a single PeerId on purpose: only the previewed chat is affected, the
rest of the app behaves exactly as before. Not persisted — cleared on restart.
*/
#pragma once

#include "data/data_peer_id.h"

namespace Fenix {

class ChatPreviewState final {
public:
	[[nodiscard]] static ChatPreviewState &Instance();

	void setActive(PeerId peerId);
	void clear();

	// True while the given chat is open in ghost preview — callers must skip
	// sending any read receipt for it.
	[[nodiscard]] bool isPreview(PeerId peerId) const;

private:
	ChatPreviewState() = default;
	~ChatPreviewState() = default;

	PeerId _previewPeer = PeerId();
};

[[nodiscard]] inline ChatPreviewState &GetChatPreview() {
	return ChatPreviewState::Instance();
}

} // namespace Fenix
