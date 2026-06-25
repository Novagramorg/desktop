/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/chat_preview_state.h"

namespace Fenix {

ChatPreviewState &ChatPreviewState::Instance() {
	static ChatPreviewState instance;
	return instance;
}

void ChatPreviewState::setActive(PeerId peerId) {
	_previewPeer = peerId;
}

void ChatPreviewState::clear() {
	_previewPeer = PeerId();
}

bool ChatPreviewState::isPreview(PeerId peerId) const {
	return _previewPeer && (_previewPeer == peerId);
}

} // namespace Fenix
