/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Join with all accounts — joins a public channel/group on every other
logged-in account, resolving it per account (each needs its own access hash).
*/
#pragma once

class ChannelData;

namespace Window {
class SessionController;
} // namespace Window

namespace Fenix {

void JoinChannelOnAllAccounts(
	not_null<Window::SessionController*> controller,
	not_null<ChannelData*> channel);

} // namespace Fenix
