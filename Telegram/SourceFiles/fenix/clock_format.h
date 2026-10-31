/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Clock text with optional seconds. Telegram Desktop formats times through
Ui::FormatDialogsDate and QLocale in the lower td_ui layer, which cannot reach
Novagram settings, so the app-layer call sites go through these helpers instead.
Equivalent of the Android org.fenixuz.utils.TimeWithSeconds hook.
*/
#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QString>

namespace Fenix {

// Locale short time, with ":ss" added when "Show seconds" is on.
[[nodiscard]] QString ClockText(QTime time);

// Ui::FormatDialogsDate with the same rules, seconds included when on.
[[nodiscard]] QString DialogsDateText(const QDateTime &lastTime);

// Changes when the setting flips, so cached texts can be invalidated.
[[nodiscard]] int ClockSerial();

} // namespace Fenix
