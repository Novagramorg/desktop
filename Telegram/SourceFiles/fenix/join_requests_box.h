/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Join requests — an on-demand box that bulk-approves or declines all pending
join requests in a channel/group the user manages.
*/
#pragma once

class PeerData;

namespace Ui {
class GenericBox;
} // namespace Ui

namespace Window {
class SessionController;
} // namespace Window

namespace Fenix {

void JoinRequestsBox(
	not_null<Ui::GenericBox*> box,
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer);

} // namespace Fenix
