/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Deep links for the Novagram settings sections: registers the tg://settings/...
paths with the app router and adds a "Copy link" context menu to a settings row,
so a link that opens and highlights that exact row can be shared. Equivalent of
the Android FenixSettings long-press copy/share of tg://novagram_settings/<key>.
*/
#pragma once

#include "settings/settings_type.h"

namespace Core::DeepLinks {
class Router;
} // namespace Core::DeepLinks

namespace Ui {
class SettingsButton;
} // namespace Ui

namespace Window {
class SessionController;
} // namespace Window

namespace Fenix {

void RegisterDeepLinks(Core::DeepLinks::Router &router);

void SetupCopyDeepLink(
	not_null<Ui::SettingsButton*> button,
	not_null<Window::SessionController*> controller,
	::Settings::Type section,
	const QString &controlId);

} // namespace Fenix
