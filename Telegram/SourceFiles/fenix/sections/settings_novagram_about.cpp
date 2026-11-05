/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/sections/settings_novagram_about.h"

#include "boxes/about_box.h"
#include "core/file_utilities.h"
#include "fenix/fenix_deep_links.h"
#include "lang/lang_keys.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "settings/settings_common_session.h"
#include "settings/sections/settings_main.h"
#include "ui/painter.h"
#include "ui/rp_widget.h"
#include "ui/ui_utility.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "window/window_session_controller_link_info.h"
#include "styles/style_fenix.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>

namespace Settings {
namespace {

using namespace Builder;

constexpr auto kChannelUz = "Novagramtg";
constexpr auto kChannelRu = "Novagram_Ru";
constexpr auto kSupport = "Avazbekedu";
constexpr auto kSiteUrl = "https://novagram.org";
constexpr auto kCompanyUrl = "https://vipads.uz";
constexpr auto kEmail = "admin@novagram.org";

[[nodiscard]] QString ControlId(const QString &name) {
	return u"novagram/about/"_q + name;
}

// The rows are two lines tall: the bottom padding holds the value line.
[[nodiscard]] const style::SettingsButton &RowStyle() {
	static const auto result = [] {
		auto st = st::novagramAboutRow;
		st.padding.setBottom(st.padding.bottom()
			+ st::novagramAboutValueSkip
			+ st::novagramAboutValueFont->height);
		return st;
	}();
	return result;
}

// The value under the row's title, on the full width of the row, so a
// username or an address is never cut by a long title.
void AddValueLine(
		not_null<Ui::SettingsButton*> button,
		const QString &value) {
	const auto &st = RowStyle();
	const auto font = st::novagramAboutValueFont;
	const auto line = Ui::CreateChild<Ui::RpWidget>(button.get());
	line->setAttribute(Qt::WA_TransparentForMouseEvents);
	button->widthValue() | rpl::on_next([=, &st](int width) {
		line->setGeometry(
			st.padding.left(),
			st.padding.top() + st.height + st::novagramAboutValueSkip,
			std::max(width - st.padding.left() - st.padding.right(), 0),
			font->height);
	}, line->lifetime());
	line->paintRequest() | rpl::on_next([=] {
		auto p = Painter(line);
		p.setFont(font);
		p.setPen(st::novagramAboutValueFg);
		p.drawTextLeft(
			0,
			0,
			line->width(),
			font->elided(value, line->width()));
	}, line->lifetime());
	line->show();
}

void AddRow(
		SectionBuilder &builder,
		const QString &name,
		rpl::producer<QString> title,
		const QString &value,
		const style::icon *icon,
		Fn<void()> click) {
	const auto id = ControlId(name);
	const auto controller = builder.controller();
	const auto button = builder.addButton({
		.id = id,
		.title = std::move(title),
		.st = &RowStyle(),
		.icon = { icon },
		.onClick = std::move(click),
		.keywords = { u"about"_q, u"novagram"_q },
	});
	if (!button) {
		return;
	}
	AddValueLine(button, value);
	if (controller) {
		Fenix::SetupCopyDeepLink(button, controller, NovagramAboutId(), id);
	}
}

void AddCentered(
		not_null<Ui::VerticalLayout*> container,
		rpl::producer<QString> text,
		const style::FlatLabel &st) {
	container->add(
		object_ptr<Ui::FlatLabel>(container, std::move(text), st),
		st::boxRowPadding,
		style::al_top);
}

void BuildHeader(SectionBuilder &builder) {
	const auto container = builder.container();
	if (!container) {
		return;
	}
	Ui::AddSkip(container, st::novagramAboutLogoTop);

	const auto size = st::novagramAboutLogoSize;
	const auto logo = container->add(
		object_ptr<Ui::RpWidget>(container),
		style::margins(),
		style::al_top);
	logo->resize(size, size);
	const auto ratio = style::DevicePixelRatio();
	const auto image = logo->lifetime().make_state<QImage>(
		QImage(u":/gui/art/logo_256.png"_q).scaled(
			size * ratio,
			size * ratio,
			Qt::IgnoreAspectRatio,
			Qt::SmoothTransformation));
	image->setDevicePixelRatio(ratio);
	logo->paintRequest() | rpl::on_next([=] {
		// The layout stretches the widget to the full width: keep the logo
		// in the middle, under it the centered name and tagline.
		auto p = QPainter(logo);
		const auto left = (logo->width() - size) / 2;
		p.drawImage(QRect(left, 0, size, size), *image);
	}, logo->lifetime());

	Ui::AddSkip(container, st::novagramAboutNameTop);
	AddCentered(
		container,
		rpl::single(u"Novagram"_q),
		st::novagramAboutName);
	Ui::AddSkip(container, st::novagramAboutTaglineTop);
	AddCentered(
		container,
		tr::lng_fenix_about_tagline(),
		st::novagramAboutTagline);
	Ui::AddSkip(container, st::novagramAboutHeaderBottom);
	Ui::AddDivider(container);
}

void BuildVersion(SectionBuilder &builder) {
	const auto controller = builder.controller();
	const auto version = currentVersionText();

	builder.addSkip();
	AddRow(
		builder,
		u"version"_q,
		tr::lng_fenix_about_version(),
		version,
		&st::menuIconInfo,
		[=] {
			if (!controller) {
				return;
			}
			QGuiApplication::clipboard()->setText(version);
			controller->showToast(
				tr::lng_fenix_about_version_copied(tr::now));
		});
	builder.addSkip();
	builder.addDivider();
}

void BuildChannels(SectionBuilder &builder) {
	const auto controller = builder.controller();
	const auto open = [=](const QString &username) {
		return [=] {
			if (controller) {
				controller->showPeerByLink({ .usernameOrId = username });
			}
		};
	};

	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_about_channels());
	AddRow(
		builder,
		u"channel_uz"_q,
		tr::lng_fenix_about_channel_uz(),
		u"@"_q + QString::fromLatin1(kChannelUz),
		&st::menuIconChannel,
		open(QString::fromLatin1(kChannelUz)));
	AddRow(
		builder,
		u"channel_ru"_q,
		tr::lng_fenix_about_channel_ru(),
		u"@"_q + QString::fromLatin1(kChannelRu),
		&st::menuIconChannel,
		open(QString::fromLatin1(kChannelRu)));
	builder.addSkip();
	builder.addDivider();
}

void BuildSupport(SectionBuilder &builder) {
	const auto controller = builder.controller();

	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_about_support());
	AddRow(
		builder,
		u"support"_q,
		tr::lng_fenix_about_support_contact(),
		u"@"_q + QString::fromLatin1(kSupport),
		&st::menuIconProfile,
		[=] {
			if (controller) {
				controller->showPeerByLink({
					.usernameOrId = QString::fromLatin1(kSupport),
				});
			}
		});
	builder.addSkip();
	builder.addDivider();
}

void BuildCompany(SectionBuilder &builder) {
	const auto controller = builder.controller();
	const auto email = QString::fromLatin1(kEmail);

	builder.addSkip();
	builder.addSubsectionTitle(tr::lng_fenix_about_company());
	AddRow(
		builder,
		u"site"_q,
		tr::lng_fenix_about_site(),
		u"novagram.org"_q,
		&st::menuIconLink,
		[] { File::OpenUrl(QString::fromLatin1(kSiteUrl)); });
	AddRow(
		builder,
		u"company_site"_q,
		tr::lng_fenix_about_company_site(),
		u"vipads.uz"_q,
		&st::menuIconLink,
		[] { File::OpenUrl(QString::fromLatin1(kCompanyUrl)); });
	AddRow(
		builder,
		u"email"_q,
		tr::lng_fenix_about_email(),
		email,
		&st::menuIconCopy,
		[=] {
			if (!controller) {
				return;
			}
			QGuiApplication::clipboard()->setText(email);
			controller->showToast(tr::lng_fenix_about_email_copied(tr::now));
		});
	builder.addSkip();
	builder.addDividerText(tr::lng_fenix_about_company_text());
}

class NovagramAboutSection final : public Section<NovagramAboutSection> {
public:
	NovagramAboutSection(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

const auto kMeta = BuildHelper({
	.id = NovagramAboutSection::Id(),
	.parentId = MainId(),
	.title = &tr::lng_fenix_about,
	.icon = &st::menuIconInfo,
}, [](SectionBuilder &builder) {
	BuildHeader(builder);
	BuildVersion(builder);
	BuildChannels(builder);
	BuildSupport(builder);
	BuildCompany(builder);
});

const SectionBuildMethod kNovagramAboutSection = kMeta.build;

NovagramAboutSection::NovagramAboutSection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> NovagramAboutSection::title() {
	return tr::lng_fenix_about();
}

void NovagramAboutSection::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kNovagramAboutSection);
	Ui::ResizeFitChild(this, content);
}

} // namespace

Type NovagramAboutId() {
	return NovagramAboutSection::Id();
}

} // namespace Settings
