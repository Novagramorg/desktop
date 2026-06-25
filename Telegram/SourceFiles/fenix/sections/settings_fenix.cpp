/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/sections/settings_fenix.h"

#include "fenix/fenix_settings.h"
#include "lang/lang_keys.h"
#include "settings/settings_builder.h"
#include "settings/settings_common_session.h"
#include "settings/sections/settings_main.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

namespace Settings {
namespace {

using namespace Builder;

void BuildGhostModeRow(SectionBuilder &builder) {
	const auto container = builder.container();

	AddSkip(container);
	AddSubsectionTitle(container, tr::lng_fenix_section_privacy());

	const auto button = AddButtonWithIcon(
		container,
		tr::lng_fenix_ghost_mode(),
		st::settingsButton,
		{ &st::menuIconStealth });
	button->toggleOn(Fenix::Get().ghostModeValue());
	button->toggledChanges(
	) | rpl::filter([](bool checked) {
		return checked != Fenix::Get().ghostMode();
	}) | rpl::on_next([=](bool checked) {
		Fenix::Get().setGhostMode(checked);
	}, button->lifetime());

	AddDividerText(container, tr::lng_fenix_ghost_mode_about());
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

	AddDividerText(container, tr::lng_fenix_save_deleted_msg_about());
}

void BuildSaveEditHistoryRow(SectionBuilder &builder) {
	const auto container = builder.container();

	AddSkip(container);

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

	AddDividerText(container, tr::lng_fenix_save_edit_history_about());
}

void BuildStoryDownloadRow(SectionBuilder &builder) {
	const auto container = builder.container();

	AddSkip(container);

	const auto button = AddButtonWithIcon(
		container,
		tr::lng_fenix_story_download(),
		st::settingsButton,
		{ &st::menuIconDownload });
	button->toggleOn(Fenix::Get().storyDownloadValue());
	button->toggledChanges(
	) | rpl::filter([](bool checked) {
		return checked != Fenix::Get().storyDownload();
	}) | rpl::on_next([=](bool checked) {
		Fenix::Get().setStoryDownload(checked);
	}, button->lifetime());

	AddDividerText(container, tr::lng_fenix_story_download_about());
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
	BuildGhostModeRow(builder);
	BuildSaveDeletedMsgRow(builder);
	BuildSaveEditHistoryRow(builder);
	BuildStoryDownloadRow(builder);
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
