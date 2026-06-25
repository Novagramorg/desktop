/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.

A real, full-screen section (not a dialog) that shows the saved edit history of
a message as chat-style bubbles, with the edit time in the bottom-right corner
of each bubble. Equivalent of the Android org.fenixuz.ui.message_history screen.
*/
#pragma once

#include "data/data_msg_id.h"

class PeerData;

namespace Window {
class SessionController;
} // namespace Window

namespace Fenix {

// Pushes the edit-history screen onto the navigation stack.
void ShowEditHistory(
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer,
	MsgId msgId);

} // namespace Fenix
