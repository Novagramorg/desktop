/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Voice changer for voice messages and calls: the effect pickers, opened from
the Novagram settings, by a right click on the microphone button or by the
button above a locked recording.
*/
#pragma once

namespace style {
struct RippleAnimation;
} // namespace style

namespace Ui {
class AbstractButton;
class GenericBox;
class RpWidget;
class Show;
} // namespace Ui

namespace Fenix::Voice {

class CallVoiceSource;

// The chosen effect's name, as the settings row shows it ("Off" for none).
[[nodiscard]] QString ChosenTitle();
[[nodiscard]] rpl::producer<QString> ChosenTitleValue();

// Whether an effect is on for voice messages.
[[nodiscard]] bool ChosenIsOn();
[[nodiscard]] rpl::producer<bool> ChosenIsOnValue();

// The round button above a locked recording's pause button (and its
// view-once button), as on Android: [paintCircle] paints the lock's circle
// in the given rect and returns the inside, where the icon goes in [fg],
// white on a filled circle while an effect is on.
[[nodiscard]] std::unique_ptr<Ui::AbstractButton> CreateRecordingButton(
	not_null<Ui::RpWidget*> parent,
	const style::RippleAnimation &ripple,
	int size,
	QMargins rippleMargins,
	Fn<QRect(QPainter&, QRect)> paintCircle,
	const style::color &fg);

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

// The effect for calls, for a call's microphone (see voice_call_adm.h): it
// follows the choice, also during the call, for as long as [lifetime] lives.
// Main thread.
[[nodiscard]] std::shared_ptr<CallVoiceSource> TrackCallVoice(
	rpl::lifetime &lifetime);

// Calls keep a choice of their own, without the tape speeds.
[[nodiscard]] QString CallTitle();
[[nodiscard]] rpl::producer<QString> CallTitleValue();

// The effects for calls; [inCall] gives the box a call window's colors.
void CallVoiceChangerBox(not_null<Ui::GenericBox*> box, bool inCall);

} // namespace Fenix::Voice
