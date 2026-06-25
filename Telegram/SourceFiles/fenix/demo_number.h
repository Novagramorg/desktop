/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.

Demo number support for Microsoft Store / store reviewers.
Equivalent of Android org.pro.utils.DemoNumber.

If the phone number entered during login matches kDemoNumber, the SMS
code is fetched from kDemoCodeUrl and auto-filled, so reviewers can sign
in to the test account without holding the SIM card themselves.
*/
#pragma once

#include <QtCore/QString>

namespace Fenix::DemoNumber {

[[nodiscard]] const QString &Number();
[[nodiscard]] bool IsDemo(const QString &phone);

// Calls `done` on the Qt main thread once the code has been fetched
// (or the request failed — in which case the result is empty).
void FetchSmsCode(Fn<void(QString code)> done);

} // namespace Fenix::DemoNumber
