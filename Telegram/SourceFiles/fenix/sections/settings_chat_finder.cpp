/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/sections/settings_chat_finder.h"

#include "apiwrap.h"
#include "base/timer.h"
#include "base/weak_ptr.h"
#include "data/data_session.h"
#include "lang/lang_keys.h"
#include "lottie/lottie_icon.h"
#include "main/main_session.h"
#include "settings/settings_common.h"
#include "settings/settings_common_session.h"
#include "ui/effects/animations.h"
#include "ui/rp_widget.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "window/window_session_controller_link_info.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"

namespace Settings {
namespace {

[[nodiscard]] QString SanitizeUsername(QString name) {
	name = name.trimmed();
	if (name.startsWith('@')) {
		name = name.mid(1);
	}
	const auto tme = name.indexOf(u"t.me/"_q, 0, Qt::CaseInsensitive);
	if (tme >= 0) {
		name = name.mid(tme + 5);
	}
	// Keep just the username token (drop any trailing path / query), lowercase.
	return name.split('/').front().split('?').front().trimmed().toLower();
}

class FenixChatFinderSection final
	: public Section<FenixChatFinderSection> {
public:
	FenixChatFinderSection(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

FenixChatFinderSection::FenixChatFinderSection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> FenixChatFinderSection::title() {
	return tr::lng_fenix_chat_finder();
}

void FenixChatFinderSection::setupContent() {
	struct State {
		mtpRequestId requestId = 0;
		QString resolved;
		base::Timer debounce;
	};

	const auto controller = this->controller();
	const auto session = &controller->session();
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	const auto state = content->lifetime().make_state<State>();

	const auto rowPadding = style::margins(
		st::boxRowPadding.left(),
		0,
		st::boxRowPadding.right(),
		0);

	// Animated search icon on top.
	Ui::AddSkip(content, 24);
	auto icon = CreateLottieIcon(
		content,
		{ .name = u"search"_q, .sizeOverride = QSize(100, 100) },
		style::margins());
	const auto animate = std::move(icon.animate);
	content->add(
		std::move(icon.widget),
		style::margins(0, 0, 0, 8),
		style::al_top);

	// Centered description.
	content->add(
		object_ptr<Ui::FlatLabel>(
			content,
			tr::lng_fenix_chat_finder_about(),
			st::confirmInviteAbout),
		rowPadding,
		style::al_top);

	// Username field.
	const auto field = content->add(
		object_ptr<Ui::InputField>(
			content,
			st::defaultInputField,
			Ui::InputField::Mode::SingleLine,
			tr::lng_fenix_chat_finder_hint()),
		style::margins(
			st::boxRowPadding.left(),
			20,
			st::boxRowPadding.right(),
			0));

	// Live availability status (green = found, red = not found).
	const auto status = content->add(
		object_ptr<Ui::FlatLabel>(content, QString(), st::confirmInviteAbout),
		style::margins(
			st::boxRowPadding.left(),
			10,
			st::boxRowPadding.right(),
			0),
		style::al_top);
	status->hide();

	const auto setStatus = [=](const QString &text, const style::color &color) {
		if (text.isEmpty()) {
			status->hide();
			return;
		}
		status->setText(text);
		status->setTextColorOverride(color->c);
		status->show();
	};

	const auto resolve = [=] {
		const auto name = SanitizeUsername(field->getLastText());
		if (state->requestId) {
			session->api().request(state->requestId).cancel();
			state->requestId = 0;
		}
		state->resolved = QString();
		if (name.size() < 4) {
			setStatus(QString(), st::boxTextFgError);
			return;
		}
		state->requestId = session->api().request(MTPcontacts_ResolveUsername(
			MTP_flags(0),
			MTP_string(name),
			MTP_string()
		)).done(crl::guard(this, [=](const MTPcontacts_ResolvedPeer &result) {
			state->requestId = 0;
			result.match([&](const MTPDcontacts_resolvedPeer &data) {
				session->data().processUsers(data.vusers());
				session->data().processChats(data.vchats());
				const auto peerId = peerFromMTP(data.vpeer());
				const auto peer = peerId
					? session->data().peerLoaded(peerId)
					: nullptr;
				if (peer) {
					state->resolved = name;
					setStatus(
						tr::lng_fenix_chat_finder_available(tr::now)
							+ u"  @"_q + name,
						st::boxTextFgGood);
				} else {
					setStatus(
						tr::lng_fenix_chat_finder_unavailable(tr::now)
							+ u"  @"_q + name,
						st::boxTextFgError);
				}
			});
		})).fail(crl::guard(this, [=] {
			state->requestId = 0;
			setStatus(
				tr::lng_fenix_chat_finder_unavailable(tr::now)
					+ u"  @"_q + name,
				st::boxTextFgError);
		})).send();
	};
	state->debounce.setCallback(resolve);

	field->changes() | rpl::on_next([=] {
		state->debounce.callOnce(350);
	}, field->lifetime());

	const auto open = [=] {
		const auto name = !state->resolved.isEmpty()
			? state->resolved
			: SanitizeUsername(field->getLastText());
		if (name.isEmpty()) {
			return;
		}
		controller->showPeerByLink({ .usernameOrId = name });
	};
	field->submits() | rpl::on_next([=](auto) { open(); }, field->lifetime());

	// Full-width "Open" button.
	Ui::AddSkip(content, 24);
	const auto button = content->add(
		object_ptr<Ui::RoundButton>(
			content,
			tr::lng_fenix_chat_finder_open(),
			st::defaultActiveButton),
		style::margins(),
		style::al_top);
	content->widthValue() | rpl::on_next([=](int width) {
		const auto inner = width
			- st::boxRowPadding.left()
			- st::boxRowPadding.right();
		button->setFullWidth(std::max(inner, 0));
	}, button->lifetime());
	button->setClickedCallback(open);
	Ui::AddSkip(content, 16);

	// Play the icon and focus the field once the screen is shown.
	showFinishes() | rpl::on_next([=] {
		animate(anim::repeat::once);
		field->setFocusFast();
	}, content->lifetime());

	Ui::ResizeFitChild(this, content);
}

} // namespace

Type FenixChatFinderId() {
	return FenixChatFinderSection::Id();
}

} // namespace Settings
