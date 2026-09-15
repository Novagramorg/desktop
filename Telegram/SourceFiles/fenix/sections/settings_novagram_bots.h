/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

"Novagram bots" — a dedicated full-screen settings section reachable from the
main Settings list (right below Novagram Features). It shows a curated, grouped
list of the project's own bots (avatar + name + @username); tapping a row opens
that bot's chat with the native START button.
*/
#pragma once

#include "settings/settings_type.h"

namespace Settings {

[[nodiscard]] Type NovagramBotsId();

} // namespace Settings
