/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/sections/settings_fenix.h"

#include "fenix/fenix_settings.h"
#include "fenix/chat_lock.h"
#include "fenix/chat_lock_box.h"
#include "main/main_session.h"
#include "main/main_session_settings.h"
#include "data/data_auto_download.h"
#include "data/data_session.h"
#include "data/data_peer.h"
#include "lang/lang_keys.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/fields/password_input.h"
#include "ui/widgets/labels.h"
#include "ui/rp_widget.h"
#include "ui/ui_utility.h"
#include "settings/settings_builder.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "settings/settings_common.h"
#include "settings/settings_common_session.h"
#include "settings/sections/settings_main.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "styles/style_share_box.h"
#include "styles/style_widgets.h"

namespace Settings {
namespace {

using namespace Builder;

void BuildStopAutoDownloadRow(SectionBuilder &builder) {
	const auto container = builder.container();
	const auto session = &builder.controller()->session();

	AddSkip(container);
	AddSubsectionTitle(container, tr::lng_fenix_section_data());

	// Directly drive Telegram's own auto-download config — the same one under
	// Settings > Advanced > Automatic media download. Turning the toggle on
	// forces every type (photo/video/file/...) off for every source (private/
	// group/channel); turning it off restores the user's previous config.
	const auto apply = [=](bool stop) {
		using Full = Data::AutoDownload::Full;
		auto &settings = session->settings();
		if (stop) {
			const auto serialized = settings.autoDownload().serialize();
			Fenix::Get().setAutoDownloadBackup(
				QString::fromLatin1(serialized.toBase64()));
			settings.autoDownload() = Full::FullDisabled();
		} else {
			const auto raw = QByteArray::fromBase64(
				Fenix::Get().autoDownloadBackup().toLatin1());
			auto restored = Full();
			if (!raw.isEmpty() && restored.setFromSerialized(raw)) {
				settings.autoDownload() = std::move(restored);
			} else {
				// No backup — re-enable every source/type at the max limit.
				auto full = Full();
				for (auto s = 0; s != Data::AutoDownload::kSourcesCount; ++s) {
					for (auto t = 0; t != Data::AutoDownload::kTypesCount; ++t) {
						full.setBytesLimit(
							static_cast<Data::AutoDownload::Source>(s),
							static_cast<Data::AutoDownload::Type>(t),
							Data::AutoDownload::kMaxBytesLimit);
					}
				}
				settings.autoDownload() = std::move(full);
			}
		}
		session->saveSettingsDelayed();
	};

	const auto button = AddButtonWithIcon(
		container,
		tr::lng_fenix_stop_auto_download(),
		st::settingsButton,
		{ &st::menuIconDownloadOff });
	button->toggleOn(Fenix::Get().stopAutoDownloadValue());
	button->toggledChanges(
	) | rpl::filter([](bool checked) {
		return checked != Fenix::Get().stopAutoDownload();
	}) | rpl::on_next([=](bool checked) {
		Fenix::Get().setStopAutoDownload(checked);
		apply(checked);
	}, button->lifetime());

	AddSkip(container);
	AddDivider(container);
}

void BuildSaveDeletedMsgRow(SectionBuilder &builder) {
	const auto container = builder.container();

	AddSkip(container);
	AddSubsectionTitle(container, tr::lng_fenix_section_messages());

	const auto button = AddButtonWithIcon(
		container,
		tr::lng_fenix_save_deleted_msg(),
		st::settingsButton,
		{ &st::menuIconRestore });
	button->toggleOn(Fenix::Get().saveDeletedMsgValue());
	button->toggledChanges(
	) | rpl::filter([](bool checked) {
		return checked != Fenix::Get().saveDeletedMsg();
	}) | rpl::on_next([=](bool checked) {
		Fenix::Get().setSaveDeletedMsg(checked);
	}, button->lifetime());
}

void BuildSaveEditHistoryRow(SectionBuilder &builder) {
	const auto container = builder.container();

	const auto button = AddButtonWithIcon(
		container,
		tr::lng_fenix_save_edit_history(),
		st::settingsButton,
		{ &st::menuIconEdit });
	button->toggleOn(Fenix::Get().saveEditHistoryValue());
	button->toggledChanges(
	) | rpl::filter([](bool checked) {
		return checked != Fenix::Get().saveEditHistory();
	}) | rpl::on_next([=](bool checked) {
		Fenix::Get().setSaveEditHistory(checked);
	}, button->lifetime());

	AddSkip(container);
	AddDivider(container);
}

void BuildHideFolderTabsRow(SectionBuilder &builder) {
	const auto container = builder.container();

	AddSkip(container);
	AddSubsectionTitle(container, tr::lng_fenix_section_interface());

	const auto button = AddButtonWithIcon(
		container,
		tr::lng_fenix_hide_folder_tabs(),
		st::settingsButton,
		{ &st::menuIconShowInChat });
	button->toggleOn(Fenix::Get().hideFolderTabsValue());
	button->toggledChanges(
	) | rpl::filter([](bool checked) {
		return checked != Fenix::Get().hideFolderTabs();
	}) | rpl::on_next([=](bool checked) {
		Fenix::Get().setHideFolderTabs(checked);
	}, button->lifetime());

	AddSkip(container);
	AddDivider(container);
}

void BuildConfirmSendRows(SectionBuilder &builder) {
	const auto container = builder.container();

	AddSkip(container);
	AddSubsectionTitle(container, tr::lng_fenix_section_confirm());

	const auto add = [&](
			tr::phrase<> label,
			const style::icon *icon,
			bool (Fenix::Settings::*getter)() const,
			void (Fenix::Settings::*setter)(bool),
			rpl::producer<bool> (Fenix::Settings::*value)() const) {
		const auto button = AddButtonWithIcon(
			container,
			label(),
			st::settingsButton,
			{ icon });
		button->toggleOn((Fenix::Get().*value)());
		button->toggledChanges(
		) | rpl::filter([=](bool checked) {
			return checked != (Fenix::Get().*getter)();
		}) | rpl::on_next([=](bool checked) {
			(Fenix::Get().*setter)(checked);
		}, button->lifetime());
	};
	add(tr::lng_fenix_confirm_voice_toggle, &st::menuIconUnmute,
		&Fenix::Settings::confirmVoice,
		&Fenix::Settings::setConfirmVoice,
		&Fenix::Settings::confirmVoiceValue);
	add(tr::lng_fenix_confirm_sticker_toggle, &st::menuIconStickers,
		&Fenix::Settings::confirmSticker,
		&Fenix::Settings::setConfirmSticker,
		&Fenix::Settings::confirmStickerValue);
	add(tr::lng_fenix_confirm_gif_toggle, &st::menuIconGif,
		&Fenix::Settings::confirmGif,
		&Fenix::Settings::setConfirmGif,
		&Fenix::Settings::confirmGifValue);

	AddSkip(container);
	AddDivider(container);
}

void FenixAutoAnswerTextBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(tr::lng_fenix_auto_answer_text());

	const auto field = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::shareComment,
		Ui::InputField::Mode::MultiLine,
		tr::lng_fenix_auto_answer_placeholder()));
	field->setText(Fenix::Get().autoAnswerText());
	field->setMaxLength(4096);
	box->setFocusCallback([=] { field->setFocusFast(); });

	box->addButton(tr::lng_settings_save(), [=] {
		Fenix::Get().setAutoAnswerText(field->getLastText());
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

void BuildAutoAnswerRows(SectionBuilder &builder) {
	const auto container = builder.container();
	const auto controller = builder.controller();

	AddSkip(container);
	AddSubsectionTitle(container, tr::lng_fenix_auto_answer_title());

	const auto toggle = AddButtonWithIcon(
		container,
		tr::lng_fenix_auto_answer(),
		st::settingsButton,
		{ &st::menuIconReply });
	toggle->toggleOn(Fenix::Get().autoAnswerValue());
	toggle->toggledChanges(
	) | rpl::filter([](bool checked) {
		return checked != Fenix::Get().autoAnswer();
	}) | rpl::on_next([=](bool checked) {
		Fenix::Get().setAutoAnswer(checked);
	}, toggle->lifetime());

	const auto edit = AddButtonWithIcon(
		container,
		tr::lng_fenix_auto_answer_text(),
		st::settingsButton,
		{ &st::menuIconEdit });
	edit->setClickedCallback([=] {
		controller->show(Box(FenixAutoAnswerTextBox));
	});
}

void FenixReminderDelayBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(tr::lng_fenix_reminder_delay());

	const auto field = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::defaultInputField,
		tr::lng_fenix_reminder_delay_ph()));
	field->setText(QString::number(Fenix::Get().messageReminderDelay()));
	box->setFocusCallback([=] { field->setFocusFast(); });

	box->addButton(tr::lng_settings_save(), [=] {
		const auto minutes = field->getLastText().trimmed().toInt();
		Fenix::Get().setMessageReminderDelay(minutes > 0 ? minutes : 5);
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

void BuildChatLockRows(SectionBuilder &builder) {
	const auto container = builder.container();
	const auto controller = builder.controller();

	AddSkip(container);
	AddSubsectionTitle(container, tr::lng_fenix_chat_lock_section());

	const auto change = AddButtonWithIcon(
		container,
		tr::lng_fenix_chat_lock_change_common(),
		st::settingsButton,
		{ &st::menuIconLock });
	change->setClickedCallback([=] {
		controller->show(Box(Fenix::ChatLockCommonPasswordBox, [=] {
			controller->showToast(tr::lng_fenix_chat_lock_changed(tr::now));
		}));
	});
}

void BuildMessageReminderRows(SectionBuilder &builder) {
	const auto container = builder.container();
	const auto controller = builder.controller();

	AddSkip(container);
	AddSubsectionTitle(container, tr::lng_fenix_reminder_title());

	const auto toggle = AddButtonWithIcon(
		container,
		tr::lng_fenix_reminder(),
		st::settingsButton,
		{ &st::menuIconUnmute });
	toggle->toggleOn(Fenix::Get().messageReminderValue());
	toggle->toggledChanges(
	) | rpl::filter([](bool checked) {
		return checked != Fenix::Get().messageReminder();
	}) | rpl::on_next([=](bool checked) {
		Fenix::Get().setMessageReminder(checked);
	}, toggle->lifetime());

	const auto delay = AddButtonWithIcon(
		container,
		tr::lng_fenix_reminder_delay(),
		st::settingsButton,
		{ &st::menuIconReschedule });
	delay->setClickedCallback([=] {
		controller->show(Box(FenixReminderDelayBox));
	});
}

class FenixSection : public Section<FenixSection> {
public:
	FenixSection(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();
};

const auto kMeta = BuildHelper({
	.id = FenixSection::Id(),
	.parentId = MainId(),
	.title = &tr::lng_fenix_features,
	.icon = &st::menuIconShowInFolder,
}, [](SectionBuilder &builder) {
	if (!builder.container()) {
		return;
	}
	BuildSaveDeletedMsgRow(builder);
	BuildSaveEditHistoryRow(builder);
	BuildStopAutoDownloadRow(builder);
	BuildHideFolderTabsRow(builder);
	BuildConfirmSendRows(builder);
	BuildAutoAnswerRows(builder);
	BuildMessageReminderRows(builder);
	BuildChatLockRows(builder);
});

const SectionBuildMethod kFenixSection = kMeta.build;

FenixSection::FenixSection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> FenixSection::title() {
	return tr::lng_fenix_features();
}

void FenixSection::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kFenixSection);
	Ui::ResizeFitChild(this, content);
}

} // namespace

Type FenixId() {
	return FenixSection::Id();
}

} // namespace Settings
