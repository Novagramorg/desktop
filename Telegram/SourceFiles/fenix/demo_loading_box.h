/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.

Modal loading box shown while fetching the demo SMS code from xmax.uz.
Equivalent of Android DemoNumber.loadingDialog().
*/
#pragma once

#include <QtCore/QPointer>

namespace Ui {
class BoxContent;
} // namespace Ui

namespace Fenix {

[[nodiscard]] QPointer<Ui::BoxContent> ShowDemoLoadingBox();

} // namespace Fenix
