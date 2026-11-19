/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Voice changer for voice messages: the effect picker, opened from the
Novagram settings.
*/
#pragma once

namespace Ui {
class GenericBox;
} // namespace Ui

namespace Fenix::Voice {

// The chosen effect's name, as the settings row shows it ("Off" for none).
[[nodiscard]] QString ChosenTitle();
[[nodiscard]] rpl::producer<QString> ChosenTitleValue();

// The effects, one radio row each in Android's order with its emoji. The
// speed row asks for one of four speeds, and Modulation, Echo and Noise for
// their settings. A pick is saved at once and closes the box.
void VoiceChangerBox(not_null<Ui::GenericBox*> box);

} // namespace Fenix::Voice
