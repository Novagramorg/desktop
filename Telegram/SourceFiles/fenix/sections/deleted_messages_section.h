/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

A chat's "Deleted messages" screen (Android DeletedMessagesScreen): the
messages others deleted in it, kept on this device, newest first, also the
ones that no longer show in the chat. Each can be copied or deleted for
good, and all of them cleared at once.
*/
#pragma once

class PeerData;

namespace Window {
class SessionController;
} // namespace Window

namespace Fenix {

void ShowDeletedMessages(
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer);

} // namespace Fenix
