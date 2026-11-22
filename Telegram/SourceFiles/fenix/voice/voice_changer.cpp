/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/voice/voice_changer.h"

#include "fenix/fenix_settings.h"
#include "fenix/voice/voice_call.h"
#include "fenix/voice/voice_effect.h"
#include "base/call_delayed.h"
#include "base/event_filter.h"
#include "base/unique_qptr.h"
#include "lang/lang_keys.h"
#include "settings/settings_common.h"
#include "ui/effects/animations.h"
#include "ui/effects/ripple_animation.h"
#include "ui/layers/generic_box.h"
#include "ui/layers/show.h"
#include "ui/painter.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/continuous_sliders.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/popup_menu.h"
#include "ui/wrap/vertical_layout.h"
#include "styles/style_boxes.h"
#include "styles/style_fenix.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"

#include <QtGui/QCursor>
#include <QtGui/QMouseEvent>

namespace Fenix::Voice {
namespace {

using Choice = Fenix::VoiceChangerChoice;

// Long enough to see the radio button move, as on Android.
constexpr auto kCloseDelay = crl::time(150);
constexpr auto kToastDuration = crl::time(2000);

[[nodiscard]] QString Emoji(int id) {
	if (id == kSpeedRow || IsSpeed(id)) {
		return u"⏩"_q;
	}
	switch (id) {
	case kOff: return u"❌"_q;
	case 8: return u"\U0001F916"_q;
	case 9: return u"\U0001F47D"_q;
	case 13: return u"\U0001F5E3️"_q;
	case kModulation: return u"\U0001F39B️"_q;
	case 1: return u"\U0001F9D2"_q;
	case 7: return u"\U0001F401"_q;
	case 3: return u"\U0001F468‍\U0001F9B0"_q;
	case 2: return u"\U0001F469‍\U0001F9B0"_q;
	case 6: return u"\U0001F47A"_q;
	case kEcho: return u"\U0001F399️"_q;
	case kNoise: return u"\U0001F4A5"_q;
	case 4:
	case 5: return u"\U0001F388"_q;
	case 12: return u"\U0001F573️"_q;
	}
	return QString();
}

[[nodiscard]] QString Name(int id) {
	switch (id) {
	case kSpeedRow: return tr::lng_fenix_voice_speed(tr::now);
	case 8: return tr::lng_fenix_voice_robot(tr::now);
	case 9: return tr::lng_fenix_voice_alien(tr::now);
	case 13: return tr::lng_fenix_voice_hoarseness(tr::now);
	case kModulation: return tr::lng_fenix_voice_modulation(tr::now);
	case 1: return tr::lng_fenix_voice_child(tr::now);
	case 7: return tr::lng_fenix_voice_mouse(tr::now);
	case 3: return tr::lng_fenix_voice_man(tr::now);
	case 2: return tr::lng_fenix_voice_woman(tr::now);
	case 6: return tr::lng_fenix_voice_monster(tr::now);
	case kEcho: return tr::lng_fenix_voice_echo(tr::now);
	case kNoise: return tr::lng_fenix_voice_noise(tr::now);
	case 4: return tr::lng_fenix_voice_helium(tr::now);
	case 5: return tr::lng_fenix_voice_hexafluoride(tr::now);
	case 12: return tr::lng_fenix_voice_cave(tr::now);
	case 16: return tr::lng_fenix_voice_very_thick(tr::now);
	case 17: return tr::lng_fenix_voice_thick(tr::now);
	case 18: return tr::lng_fenix_voice_thin(tr::now);
	case 19: return tr::lng_fenix_voice_very_thin(tr::now);
	}
	return tr::lng_fenix_voice_off(tr::now);
}

[[nodiscard]] QString ToneText(int tone) {
	return (tone > 0)
		? (u"+"_q + QString::number(tone))
		: QString::number(tone);
}

[[nodiscard]] QString PercentText(int step) {
	return QString::number(step * 10) + u"%"_q;
}

[[nodiscard]] QString SecondsText(int step) {
	return tr::lng_fenix_voice_seconds(
		tr::now,
		lt_seconds,
		QString::number(step / 10., 'f', 1));
}

// The settings as ranges allow them, as the effect uses them.
[[nodiscard]] Choice Clamped(Choice choice) {
	choice.tone = std::clamp(choice.tone, kToneMin, kToneMax);
	choice.echoDelay = std::clamp(choice.echoDelay, kStepMin, kStepMax);
	choice.echoVolume = std::clamp(choice.echoVolume, kStepMin, kStepMax);
	choice.noiseVolume = std::clamp(choice.noiseVolume, kStepMin, kStepMax);
	return choice;
}

// The picker's row of an effect: a speed is under the speed row, and an
// unknown id reads as off.
[[nodiscard]] int RowOf(int preset) {
	return IsSpeed(preset)
		? kSpeedRow
		: IsKnown(preset)
		? preset
		: kOff;
}

[[nodiscard]] QString TitleOf(int preset) {
	return Name(IsKnown(preset) ? preset : kOff);
}

[[nodiscard]] bool IsOn(int preset) {
	return IsKnown(preset) && (preset != kOff);
}

// A row's text: the emoji and the name, then the settings of the three
// effects that have them, or the chosen speed, as Android shows them under
// the name.
[[nodiscard]] QString RowText(int id, const Choice &choice) {
	auto result = Emoji(id) + u"  "_q + Name(id);
	if (id == kModulation) {
		result += u" ("_q + ToneText(choice.tone) + u")"_q;
	} else if (id == kEcho) {
		result += u" ("_q
			+ PercentText(choice.echoVolume)
			+ u", "_q
			+ SecondsText(choice.echoDelay)
			+ u")"_q;
	} else if (id == kNoise) {
		result += u" ("_q + PercentText(choice.noiseVolume) + u")"_q;
	} else if (id == kSpeedRow && IsSpeed(choice.preset)) {
		result += u": "_q + Name(choice.preset);
	}
	return result;
}

// The width of the widest of the texts of the values from [min] to [max].
[[nodiscard]] int LabelWidth(int min, int max, Fn<QString(int)> text) {
	const auto &font = st::settingsScaleLabel.style.font;
	auto result = 0;
	for (auto i = min; i <= max; ++i) {
		result = std::max(result, font->width(text(i)));
	}
	return result;
}

// A slider of the values from [min] to [max], with the value's text on its
// right in [labelWidth] (the same for the sliders of one box, so they end
// at the same place); [changed] gets every value it moves to.
void AddSlider(
		not_null<Ui::GenericBox*> box,
		const tr::phrase<> *title,
		int min,
		int max,
		int value,
		int labelWidth,
		Fn<QString(int)> text,
		Fn<void(int)> changed) {
	if (title) {
		Ui::AddSubsectionTitle(box->verticalLayout(), (*title)());
	}
	auto sliderWithLabel = ::Settings::MakeSliderWithLabel(
		box->verticalLayout(),
		st::settingsScale,
		st::settingsScaleLabel,
		st::normalFont->spacew * 2,
		labelWidth,
		true);
	const auto slider = sliderWithLabel.slider;
	const auto label = sliderWithLabel.label;
	box->addRow(
		std::move(sliderWithLabel.widget),
		st::settingsBigScalePadding);

	const auto update = [=](int now) {
		label->setText(text(now));
		changed(now);
	};
	slider->setPseudoDiscrete(
		max - min + 1,
		[=](int index) { return min + index; },
		value,
		update,
		update);
	update(value);
}

// Modulation's pitch, Echo's delay and volume, or Noise's volume. Save
// keeps them and picks the effect; Cancel leaves both as they were.
void EffectSettingsBox(
		not_null<Ui::GenericBox*> box,
		int id,
		Fn<void(Choice)> done) {
	box->setTitle(rpl::single(Name(id)));
	box->setWidth(st::boxWideWidth);

	const auto state = box->lifetime().make_state<Choice>(
		Clamped(Fenix::Get().voiceChanger()));
	if (id == kModulation) {
		AddSlider(
			box,
			nullptr,
			kToneMin,
			kToneMax,
			state->tone,
			LabelWidth(kToneMin, kToneMax, ToneText),
			ToneText,
			[=](int value) { state->tone = value; });
		box->addRow(
			object_ptr<Ui::FlatLabel>(
				box,
				tr::lng_fenix_voice_tone_about(),
				st::boxDividerLabel),
			st::boxRowPadding + style::margins(0, st::boxLittleSkip, 0, 0));
	} else if (id == kEcho) {
		const auto width = std::max(
			LabelWidth(kStepMin, kStepMax, SecondsText),
			LabelWidth(kStepMin, kStepMax, PercentText));
		AddSlider(
			box,
			&tr::lng_fenix_voice_delay,
			kStepMin,
			kStepMax,
			state->echoDelay,
			width,
			SecondsText,
			[=](int value) { state->echoDelay = value; });
		AddSlider(
			box,
			&tr::lng_fenix_voice_volume,
			kStepMin,
			kStepMax,
			state->echoVolume,
			width,
			PercentText,
			[=](int value) { state->echoVolume = value; });
	} else {
		AddSlider(
			box,
			&tr::lng_fenix_voice_volume,
			kStepMin,
			kStepMax,
			state->noiseVolume,
			LabelWidth(kStepMin, kStepMax, PercentText),
			PercentText,
			[=](int value) { state->noiseVolume = value; });
	}

	box->addButton(tr::lng_settings_save(), [=] {
		auto choice = *state;
		choice.preset = id;
		done(choice);
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

// The four tape speeds; a pick is saved and closes the box.
void SpeedBox(not_null<Ui::GenericBox*> box, Fn<void(Choice)> done) {
	box->setTitle(tr::lng_fenix_voice_speed());
	box->setWidth(st::boxWideWidth);

	const auto current = Fenix::Get().voiceChanger();
	const auto group = std::make_shared<Ui::RadiobuttonGroup>(
		IsSpeed(current.preset) ? current.preset : kOff);
	const auto picked = box->lifetime().make_state<bool>(false);
	// Closed (Escape, a click outside) before the pick was handed over:
	// the pick doesn't count.
	const auto closing = box->lifetime().make_state<bool>(false);
	box->boxClosing() | rpl::on_next([=] {
		*closing = true;
	}, box->lifetime());
	const auto pick = [=](int speed) {
		if (*picked) {
			return;
		}
		*picked = true;
		base::call_delayed(kCloseDelay, box.get(), [=] {
			if (*closing) {
				return;
			}
			auto choice = Fenix::Get().voiceChanger();
			choice.preset = speed;
			done(choice);
			box->closeBox();
		});
	};
	// A new speed comes as a change of the group: from a click, the
	// keyboard or a screen reader (those don't make a click).
	group->setChangedCallback(pick);
	for (const auto speed : SpeedIds()) {
		const auto radio = box->addRow(
			object_ptr<Ui::Radiobutton>(
				box,
				group,
				speed,
				Name(speed),
				st::defaultBoxCheckbox),
			st::boxRowPadding + style::margins(0, 0, 0, st::boxLittleSkip));
		// The speed already chosen changes nothing, so a click on it picks
		// it again.
		radio->clicks(
		) | rpl::filter([=] {
			return (group->current() == speed);
		}) | rpl::on_next([=] {
			pick(speed);
		}, radio->lifetime());
	}
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

class RecordingButton final : public Ui::RippleButton {
public:
	RecordingButton(
		not_null<Ui::RpWidget*> parent,
		const style::RippleAnimation &ripple,
		int size,
		QMargins rippleMargins,
		Fn<QRect(QPainter&, QRect)> paintCircle,
		const style::color &fg);

protected:
	QImage prepareRippleMask() const override;
	QPoint prepareRippleStartPosition() const override;

private:
	const QRect _rippleRect;
	Ui::Animations::Simple _onAnimation;
	bool _on = false;

};

RecordingButton::RecordingButton(
	not_null<Ui::RpWidget*> parent,
	const style::RippleAnimation &ripple,
	int size,
	QMargins rippleMargins,
	Fn<QRect(QPainter&, QRect)> paintCircle,
	const style::color &fg)
: RippleButton(parent, ripple)
, _rippleRect(QRect(0, 0, size, size) - rippleMargins)
, _on(ChosenIsOn()) {
	resize(size, size);

	ChosenIsOnValue(
	) | rpl::on_next([=](bool on) {
		if (_on == on) {
			return;
		}
		_on = on;
		_onAnimation.start(
			[=] { update(); },
			on ? 0. : 1.,
			on ? 1. : 0.,
			st::universalDuration);
	}, lifetime());

	ChosenTitleValue(
	) | rpl::on_next([=](const QString &title) {
		setAccessibleName(tr::lng_fenix_voice_changer_current(
			tr::now,
			lt_effect,
			title));
	}, lifetime());

	paintRequest(
	) | rpl::on_next([=] {
		auto p = QPainter(this);
		const auto inner = paintCircle(p, rect());
		paintRipple(p, _rippleRect.x(), _rippleRect.y());

		const auto on = _onAnimation.value(_on ? 1. : 0.);
		if (on > 0.) {
			auto hq = PainterHighQualityEnabler(p);
			p.setPen(Qt::NoPen);
			p.setBrush(st::historySendIconFg);
			const auto radius = st::novagramVoiceChangerOnRadius * on;
			p.drawEllipse(QRectF(inner).center(), radius, radius);
		}
		if (on < 1.) {
			p.setOpacity(1. - on);
			st::novagramVoiceChangerIcon.paintInCenter(p, inner, fg->c);
		}
		if (on > 0.) {
			p.setOpacity(on);
			st::novagramVoiceChangerIconOn.paintInCenter(p, inner);
		}
	}, lifetime());
}

QImage RecordingButton::prepareRippleMask() const {
	return Ui::RippleAnimation::EllipseMask(_rippleRect.size());
}

QPoint RecordingButton::prepareRippleStartPosition() const {
	return mapFromGlobal(QCursor::pos()) - _rippleRect.topLeft();
}

} // namespace

QString ChosenTitle() {
	return TitleOf(Fenix::Get().voiceChanger().preset);
}

rpl::producer<QString> ChosenTitleValue() {
	return Fenix::Get().voiceChangerValue(
	) | rpl::map([](const Choice &choice) {
		return TitleOf(choice.preset);
	});
}

bool ChosenIsOn() {
	return IsOn(Fenix::Get().voiceChanger().preset);
}

rpl::producer<bool> ChosenIsOnValue() {
	return Fenix::Get().voiceChangerValue(
	) | rpl::map([](const Choice &choice) {
		return IsOn(choice.preset);
	}) | rpl::distinct_until_changed();
}

std::unique_ptr<Ui::AbstractButton> CreateRecordingButton(
		not_null<Ui::RpWidget*> parent,
		const style::RippleAnimation &ripple,
		int size,
		QMargins rippleMargins,
		Fn<QRect(QPainter&, QRect)> paintCircle,
		const style::color &fg) {
	return std::make_unique<RecordingButton>(
		parent,
		ripple,
		size,
		rippleMargins,
		std::move(paintCircle),
		fg);
}

void VoiceChangerBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(tr::lng_fenix_voice_changer());
	box->setWidth(st::boxWideWidth);

	struct State {
		base::weak_qptr<Ui::GenericBox> settings;
		bool reverting = false;
		bool closing = false;
		bool hidden = false;
	};
	const auto state = box->lifetime().make_state<State>();
	box->boxClosing() | rpl::on_next([=] {
		state->hidden = true;
	}, box->lifetime());
	const auto saved = [] {
		return Fenix::Get().voiceChanger();
	};
	const auto group = std::make_shared<Ui::RadiobuttonGroup>(
		RowOf(saved().preset));
	const auto weak = base::weak_qptr<Ui::GenericBox>(box.get());

	const auto close = [=] {
		if (state->closing) {
			return;
		}
		state->closing = true;
		base::call_delayed(kCloseDelay, box.get(), [=] {
			if (!state->hidden) {
				box->closeBox();
			}
		});
	};
	// A row whose settings were cancelled gives the radio back to the
	// effect that is still chosen. Called only while this box is alive,
	// so the group (owned by its buttons) is too; holding it here would
	// make a cycle through the group's own callback.
	const auto rawGroup = group.get();
	const auto revert = [=] {
		state->reverting = true;
		rawGroup->setValue(RowOf(saved().preset));
		state->reverting = false;
	};
	const auto openSettings = [=](int row) {
		if (state->settings || state->closing) {
			return;
		}
		// The settings box closes itself after handing over a choice; it
		// may outlive this box (all layers hidden at once), so this box's
		// state is touched only while this box is alive.
		const auto chosen = std::make_shared<bool>(false);
		const auto done = [=](Choice choice) {
			if (*chosen) {
				return;
			}
			*chosen = true;
			Fenix::Get().setVoiceChanger(choice);
			if (weak) {
				close();
			}
		};
		auto settings = (row == kSpeedRow)
			? Box(SpeedBox, done)
			: Box(EffectSettingsBox, row, done);
		settings->boxClosing() | rpl::on_next([=] {
			if (!*chosen && weak) {
				revert();
			}
		}, settings->lifetime());
		state->settings = box->uiShow()->show(std::move(settings));
	};

	group->setChangedCallback([=](int row) {
		if (state->reverting || state->closing) {
			return;
		} else if (row == kSpeedRow || HasSettings(row)) {
			openSettings(row);
			return;
		}
		auto choice = saved();
		choice.preset = row;
		Fenix::Get().setVoiceChanger(choice);
		close();
	});

	const auto current = Clamped(saved());
	for (const auto id : RowIds()) {
		const auto radio = box->addRow(
			object_ptr<Ui::Radiobutton>(
				box,
				group,
				id,
				RowText(id, current),
				st::defaultBoxCheckbox),
			st::boxRowPadding + style::margins(0, 0, 0, st::boxLittleSkip));
		// A long name with its settings (Uzbek, Russian) takes two lines.
		radio->setAllowTextLines(2);
		if (id == kSpeedRow || HasSettings(id)) {
			// The settings open on every click, also on the row that is
			// already chosen (a new choice opens them as it changes).
			radio->clicks(
			) | rpl::filter([=] {
				return (group->current() == id);
			}) | rpl::on_next([=] {
				openSettings(id);
			}, radio->lifetime());
		}
	}

	box->addButton(tr::lng_box_ok(), [=] { box->closeBox(); });
}

void SetupRecordButtonMenu(
		not_null<QObject*> context,
		not_null<QWidget*> button,
		std::shared_ptr<Ui::Show> show,
		Fn<bool()> available) {
	const auto menu = std::make_shared<base::unique_qptr<Ui::PopupMenu>>();
	base::install_event_filter(context, button, [=](not_null<QEvent*> e) {
		const auto type = e->type();
		if (type == QEvent::ContextMenu) {
			if (!available() || !show->valid()) {
				return base::EventFilterResult::Continue;
			}
			*menu = base::make_unique_q<Ui::PopupMenu>(
				button,
				st::popupMenuWithIcons);
			(*menu)->addAction(
				tr::lng_fenix_voice_changer_current(
					tr::now,
					lt_effect,
					ChosenTitle()),
				[=] { show->showBox(Box(VoiceChangerBox)); },
				&st::menuIconCustomize);
			(*menu)->popup(QCursor::pos());
			return base::EventFilterResult::Cancel;
		} else if (type == QEvent::MouseButtonPress
			|| type == QEvent::MouseButtonRelease
			|| type == QEvent::MouseButtonDblClick) {
			// A right click opens the menu above; it shouldn't also start
			// a recording, as a hold on the button does.
			const auto mouse = static_cast<QMouseEvent*>(e.get());
			if (mouse->button() == Qt::RightButton && available()) {
				return base::EventFilterResult::Cancel;
			}
		}
		return base::EventFilterResult::Continue;
	});
}

void ShowRecordingToast(const std::shared_ptr<Ui::Show> &show) {
	const auto preset = Fenix::Get().voiceChanger().preset;
	if (!show || !show->valid() || !IsOn(preset)) {
		return;
	}
	show->showToast(
		Emoji(preset)
			+ u" "_q
			+ tr::lng_fenix_voice_changer_current(
				tr::now,
				lt_effect,
				Name(preset)),
		kToastDuration);
}

std::shared_ptr<CallVoiceSource> TrackCallVoice(rpl::lifetime &lifetime) {
	auto result = std::make_shared<CallVoiceSource>();
	Fenix::Get().callVoiceChangerValue(
	) | rpl::on_next([=](const Choice &) {
		result->set(ChosenForCalls());
	}, lifetime);
	return result;
}

} // namespace Fenix::Voice
