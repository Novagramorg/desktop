/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/sections/settings_fenix.h"

#include "fenix/fenix_settings.h"
#include "fenix/fenix_deep_links.h"
#include "fenix/chat_lock.h"
#include "fenix/chat_lock_box.h"
#include "fenix/saved_history.h"
#include "main/main_session.h"
#include "main/main_session_settings.h"
#include "data/data_auto_download.h"
#include "data/data_session.h"
#include "data/data_peer.h"
#include "lang/lang_keys.h"
#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/fields/password_input.h"
#include "ui/widgets/labels.h"
#include "ui/rp_widget.h"
#include "ui/text/format_values.h"
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

[[nodiscard]] QString ControlId(const QString &name) {
	return u"novagram/"_q + name;
}

// Every Novagram row goes through these two helpers so that each one gets a
// stable id: that is what makes it findable in settings search, highlightable
// when a tg://settings/novagram?highlight=... link is opened, and copyable as
// such a link from its own context menu.
Ui::SettingsButton *AddToggle(
		SectionBuilder &builder,
		const QString &name,
		rpl::producer<QString> title,
		const style::icon *icon,
		QStringList keywords,
		Fn<bool()> checked,
		Fn<void(bool)> set,
		rpl::producer<bool> value) {
	const auto id = ControlId(name);
	const auto controller = builder.controller();
	const auto button = builder.addButton({
		.id = id,
		.title = std::move(title),
		.icon = { icon },
		.toggled = std::move(value),
		.keywords = std::move(keywords),
	});
	if (!button) {
		return nullptr;
	}
	button->toggledValue(
	) | rpl::filter([=](bool now) {
		return (now != checked());
	}) | rpl::on_next([=](bool now) {
		set(now);
	}, button->lifetime());
	if (controller) {
		Fenix::SetupCopyDeepLink(button, controller, FenixId(), id);
	}
	return button;
}

Ui::SettingsButton *AddAction(
		SectionBuilder &builder,
		const QString &name,
		rpl::producer<QString> title,
		const style::icon *icon,
		QStringList keywords,
		Fn<void()> click) {
	const auto id = ControlId(name);
	const auto controller = builder.controller();
	const auto button = builder.addButton({
		.id = id,
		.title = std::move(title),
		.icon = { icon },
		.onClick = std::move(click),
		.keywords = std::move(keywords),
	});
	if (button && controller) {
		Fenix::SetupCopyDeepLink(button, controller, FenixId(), id);
	}
	return button;
}

void BuildMessagesRows(SectionBuilder &builder) {
	const auto controller = builder.controller();
	const auto session = builder.session();

	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_section_messages());

	AddToggle(
		builder,
		u"save_deleted"_q,
		tr::lng_fenix_save_deleted_msg(),
		&st::menuIconRestore,
		{ u"deleted"_q, u"messages"_q, u"save"_q },
		[] { return Fenix::Get().saveDeletedMsg(); },
		[](bool value) { Fenix::Get().setSaveDeletedMsg(value); },
		Fenix::Get().saveDeletedMsgValue());

	AddToggle(
		builder,
		u"save_edit_history"_q,
		tr::lng_fenix_save_edit_history(),
		&st::menuIconEdit,
		{ u"edit"_q, u"history"_q, u"messages"_q },
		[] { return Fenix::Get().saveEditHistory(); },
		[](bool value) { Fenix::Get().setSaveEditHistory(value); },
		Fenix::Get().saveEditHistoryValue());

	// The size of the account's saved history on its row, as on Android,
	// measured again once it is cleared.
	const auto cleared = std::make_shared<rpl::event_stream<>>();
	const auto id = ControlId(u"clear_saved_history"_q);
	const auto button = builder.addButton({
		.id = id,
		.title = tr::lng_fenix_saved_history_clear(),
		.icon = { &st::menuIconClear },
		.label = rpl::single(rpl::empty) | rpl::then(
			cleared->events()
		) | rpl::map([=] {
			return Ui::FormatSizeText(Fenix::SavedHistorySize(session));
		}),
		.onClick = [=] {
			if (!controller) {
				return;
			}
			controller->show(Ui::MakeConfirmBox({
				.text = tr::lng_fenix_saved_history_clear_sure(),
				.confirmed = [=](Fn<void()> close) {
					Fenix::ClearSavedHistory(session);
					cleared->fire({});
					controller->showToast(
						tr::lng_fenix_saved_history_cleared(tr::now));
					close();
				},
				.confirmText = tr::lng_local_storage_clear_some(),
				.confirmStyle = &st::attentionBoxButton,
			}));
		},
		.keywords = { u"clear"_q, u"deleted"_q, u"edit"_q, u"history"_q },
	});
	if (button && controller) {
		Fenix::SetupCopyDeepLink(button, controller, FenixId(), id);
	}

	builder.addSkip();
	builder.addDivider();
}

void BuildStopAutoDownloadRow(SectionBuilder &builder) {
	const auto session = builder.session();

	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_section_data());

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

	AddToggle(
		builder,
		u"stop_auto_download"_q,
		tr::lng_fenix_stop_auto_download(),
		&st::menuIconDownloadOff,
		{ u"download"_q, u"auto"_q, u"media"_q, u"traffic"_q },
		[] { return Fenix::Get().stopAutoDownload(); },
		[=](bool value) {
			Fenix::Get().setStopAutoDownload(value);
			apply(value);
		},
		Fenix::Get().stopAutoDownloadValue());

	builder.addSkip();
	builder.addDivider();
}

void BuildStoriesRows(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_section_stories());

	AddToggle(
		builder,
		u"hide_stories"_q,
		tr::lng_fenix_hide_stories(),
		&st::menuIconStoriesArchive,
		{ u"stories"_q, u"hide"_q },
		[] { return Fenix::Get().hideStories(); },
		[](bool value) { Fenix::Get().setHideStories(value); },
		Fenix::Get().hideStoriesValue());

	AddToggle(
		builder,
		u"story_ghost"_q,
		tr::lng_fenix_story_ghost(),
		&st::menuIconStealth,
		{ u"stories"_q, u"ghost"_q, u"anonymous"_q, u"stealth"_q },
		[] { return Fenix::Get().ghostStory(); },
		[](bool value) { Fenix::Get().setGhostStory(value); },
		Fenix::Get().ghostStoryValue());

	builder.addSkip();
	builder.addDividerText(tr::lng_fenix_story_ghost_about());
}

void BuildInterfaceRows(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_section_interface());

	AddToggle(
		builder,
		u"hide_folder_tabs"_q,
		tr::lng_fenix_hide_folder_tabs(),
		&st::menuIconShowInChat,
		{ u"folder"_q, u"tabs"_q, u"filters"_q, u"hide"_q },
		[] { return Fenix::Get().hideFolderTabs(); },
		[](bool value) { Fenix::Get().setHideFolderTabs(value); },
		Fenix::Get().hideFolderTabsValue());

	AddToggle(
		builder,
		u"avatar_opens_profile"_q,
		tr::lng_fenix_avatar_profile(),
		&st::menuIconProfile,
		{ u"avatar"_q, u"profile"_q, u"userpic"_q },
		[] { return Fenix::Get().avatarOpensProfile(); },
		[](bool value) { Fenix::Get().setAvatarOpensProfile(value); },
		Fenix::Get().avatarOpensProfileValue());

	AddToggle(
		builder,
		u"show_seconds"_q,
		tr::lng_fenix_show_seconds(),
		&st::menuIconReschedule,
		{ u"seconds"_q, u"time"_q, u"clock"_q },
		[] { return Fenix::Get().showSeconds(); },
		[](bool value) { Fenix::Get().setShowSeconds(value); },
		Fenix::Get().showSecondsValue());

	builder.addSkip();
	builder.addDividerText(tr::lng_fenix_avatar_profile_about());
}

void BuildConfirmSendRows(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_section_confirm());

	AddToggle(
		builder,
		u"confirm_voice"_q,
		tr::lng_fenix_confirm_voice_toggle(),
		&st::menuIconUnmute,
		{ u"confirm"_q, u"voice"_q },
		[] { return Fenix::Get().confirmVoice(); },
		[](bool value) { Fenix::Get().setConfirmVoice(value); },
		Fenix::Get().confirmVoiceValue());

	AddToggle(
		builder,
		u"confirm_sticker"_q,
		tr::lng_fenix_confirm_sticker_toggle(),
		&st::menuIconStickers,
		{ u"confirm"_q, u"sticker"_q },
		[] { return Fenix::Get().confirmSticker(); },
		[](bool value) { Fenix::Get().setConfirmSticker(value); },
		Fenix::Get().confirmStickerValue());

	AddToggle(
		builder,
		u"confirm_gif"_q,
		tr::lng_fenix_confirm_gif_toggle(),
		&st::menuIconGif,
		{ u"confirm"_q, u"gif"_q },
		[] { return Fenix::Get().confirmGif(); },
		[](bool value) { Fenix::Get().setConfirmGif(value); },
		Fenix::Get().confirmGifValue());

	builder.addSkip();
	builder.addDivider();
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
	const auto controller = builder.controller();

	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_auto_answer_title());

	AddToggle(
		builder,
		u"auto_answer"_q,
		tr::lng_fenix_auto_answer(),
		&st::menuIconReply,
		{ u"auto"_q, u"answer"_q, u"reply"_q },
		[] { return Fenix::Get().autoAnswer(); },
		[](bool value) { Fenix::Get().setAutoAnswer(value); },
		Fenix::Get().autoAnswerValue());

	AddAction(
		builder,
		u"auto_answer_text"_q,
		tr::lng_fenix_auto_answer_text(),
		&st::menuIconEdit,
		{ u"auto"_q, u"answer"_q, u"text"_q },
		[=] {
			if (controller) {
				controller->show(Box(FenixAutoAnswerTextBox));
			}
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

void BuildMessageReminderRows(SectionBuilder &builder) {
	const auto controller = builder.controller();

	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_reminder_title());

	AddToggle(
		builder,
		u"reminder"_q,
		tr::lng_fenix_reminder(),
		&st::menuIconUnmute,
		{ u"reminder"_q, u"unread"_q },
		[] { return Fenix::Get().messageReminder(); },
		[](bool value) { Fenix::Get().setMessageReminder(value); },
		Fenix::Get().messageReminderValue());

	AddAction(
		builder,
		u"reminder_delay"_q,
		tr::lng_fenix_reminder_delay(),
		&st::menuIconReschedule,
		{ u"reminder"_q, u"delay"_q, u"minutes"_q },
		[=] {
			if (controller) {
				controller->show(Box(FenixReminderDelayBox));
			}
		});
}

void BuildChatLockRows(SectionBuilder &builder) {
	const auto controller = builder.controller();

	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_chat_lock_section());

	AddAction(
		builder,
		u"chat_lock_password"_q,
		tr::lng_fenix_chat_lock_change_common(),
		&st::menuIconLock,
		{ u"lock"_q, u"password"_q, u"passcode"_q },
		[=] {
			if (!controller) {
				return;
			}
			controller->show(Box(Fenix::ChatLockCommonPasswordBox, [=] {
				controller->showToast(
					tr::lng_fenix_chat_lock_changed(tr::now));
			}));
		});
}

void BuildAutoAcceptJoinRows(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_section_channels());

	AddToggle(
		builder,
		u"auto_accept_join"_q,
		tr::lng_fenix_auto_accept_join(),
		&st::menuIconInvite,
		{ u"join"_q, u"requests"_q, u"accept"_q, u"channels"_q },
		[] { return Fenix::Get().autoAcceptJoin(); },
		[](bool value) { Fenix::Get().setAutoAcceptJoin(value); },
		Fenix::Get().autoAcceptJoinValue());

	builder.addSkip();
	builder.addDividerText(tr::lng_fenix_auto_accept_join_about());
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
	BuildMessagesRows(builder);
	BuildStoriesRows(builder);
	BuildStopAutoDownloadRow(builder);
	BuildInterfaceRows(builder);
	BuildConfirmSendRows(builder);
	BuildAutoAnswerRows(builder);
	BuildMessageReminderRows(builder);
	BuildChatLockRows(builder);
	BuildAutoAcceptJoinRows(builder);
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
