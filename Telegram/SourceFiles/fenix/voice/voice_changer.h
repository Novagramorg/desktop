/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Voice changer for voice messages: the effect picker, opened from the
Novagram settings or by a right click on the microphone button.
*/
#pragma once

namespace Ui {
class GenericBox;
class Show;
} // namespace Ui

namespace Fenix::Voice {

// The chosen effect's name, as the settings row shows it ("Off" for none).
[[nodiscard]] QString ChosenTitle();
[[nodiscard]] rpl::producer<QString> ChosenTitleValue();

// The effects, one radio row each in Android's order with its emoji. The
// speed row asks for one of four speeds, and Modulation, Echo and Noise for
// their settings. A pick is saved at once and closes the box.
void VoiceChangerBox(not_null<Ui::GenericBox*> box);

// A right click on [button] (the microphone, while [available]) opens a
// menu with the voice changer, instead of starting a recording.
void SetupRecordButtonMenu(
	not_null<QObject*> context,
	not_null<QWidget*> button,
	std::shared_ptr<Ui::Show> show,
	Fn<bool()> available);

// A short toast naming the effect as a voice message starts recording, so
// a changed voice is never sent by mistake. Nothing when none is chosen.
void ShowRecordingToast(const std::shared_ptr<Ui::Show> &show);

} // namespace Fenix::Voice
