/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

The "Default text style" picker — opened from each chat's menu. Lets the user
choose a style (Bold/Italic/Mono/…) that Novagram applies to the whole message
right before sending. The chosen style is global (stored in Fenix::Settings);
this box is just the picker UI. Mirrors the Android TextStyleDialog.
*/
#pragma once

namespace Ui {
class GenericBox;
} // namespace Ui

namespace Fenix {

void FenixTextStyleBox(not_null<Ui::GenericBox*> box);

} // namespace Fenix
