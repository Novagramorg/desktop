/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/clock_format.h"

#include "fenix/fenix_settings.h"
#include "ui/text/format_values.h"

#include <QtCore/QLocale>

#include <cmath>

namespace Fenix {

QString ClockText(QTime time) {
	if (!Get().showSeconds()) {
		return QLocale().toString(time, QLocale::ShortFormat);
	}
	// Keep the locale's own layout (24h, AM/PM, separators) and only widen
	// the minutes field, instead of hard-coding a format of our own.
	auto format = QLocale().timeFormat(QLocale::ShortFormat);
	if (!format.contains(u"ss"_q)) {
		format.replace(u"mm"_q, u"mm:ss"_q);
	}
	return QLocale().toString(time, format);
}

QString DialogsDateText(const QDateTime &lastTime) {
	if (!Get().showSeconds()) {
		return Ui::FormatDialogsDate(lastTime);
	}
	// Same "last 20 hours show a time" rule as Ui::FormatDialogsDate, which
	// lives in td_ui and cannot ask us about the setting.
	constexpr auto kRecentlyInSeconds = 20 * 3600;
	const auto now = QDateTime::currentDateTime();
	if ((lastTime.date() == now.date())
		|| (std::abs(lastTime.secsTo(now)) < kRecentlyInSeconds)) {
		return ClockText(lastTime.time());
	}
	return Ui::FormatDialogsDate(lastTime);
}

int ClockSerial() {
	return Get().showSeconds() ? 1 : 0;
}

} // namespace Fenix
