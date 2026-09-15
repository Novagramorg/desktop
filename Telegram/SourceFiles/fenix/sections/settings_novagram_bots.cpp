/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/sections/settings_novagram_bots.h"

#include "apiwrap.h"
#include "base/weak_ptr.h"
#include "boxes/peer_list_box.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "settings/settings_common.h"
#include "settings/settings_common_session.h"
#include "ui/vertical_list.h"
#include "ui/ui_utility.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_settings.h"

#include <vector>

namespace Settings {
namespace {

// A curated group of Novagram's own bots. Usernames verbatim (no leading '@').
struct BotGroup {
	rpl::producer<QString> title;
	std::vector<QString> usernames;
};

// One category's worth of bot rows. Each username is resolved lazily (raw
// contacts.resolveUsername, so a not-yet-registered bot fails silently instead
// of popping an error box). As soon as a bot resolves its row is appended, with
// the real avatar + name and an "@username" subtitle; tapping opens the bot
// chat (native START).
class BotsController final
	: public PeerListController
	, public base::has_weak_ptr {
public:
	BotsController(
		not_null<Window::SessionController*> window,
		std::vector<QString> usernames);

	Main::Session &session() const override;
	void prepare() override;
	void rowClicked(not_null<PeerListRow*> row) override;

private:
	void resolve(const QString &username);
	void appendResolved(not_null<PeerData*> peer, const QString &username);

	const not_null<Window::SessionController*> _window;
	const std::vector<QString> _usernames;

};

BotsController::BotsController(
	not_null<Window::SessionController*> window,
	std::vector<QString> usernames)
: _window(window)
, _usernames(std::move(usernames)) {
}

Main::Session &BotsController::session() const {
	return _window->session();
}

void BotsController::prepare() {
	for (const auto &username : _usernames) {
		resolve(username);
	}
}

void BotsController::resolve(const QString &username) {
	const auto session = &_window->session();
	if (const auto peer = session->data().peerByUsername(username)) {
		appendResolved(peer, username);
		return;
	}
	session->api().request(MTPcontacts_ResolveUsername(
		MTP_flags(0),
		MTP_string(username),
		MTP_string()
	)).done(crl::guard(this, [=](
			const MTPcontacts_ResolvedPeer &result) {
		result.match([&](const MTPDcontacts_resolvedPeer &data) {
			session->data().processUsers(data.vusers());
			session->data().processChats(data.vchats());
			const auto peerId = peerFromMTP(data.vpeer());
			const auto peer = peerId
				? session->data().peerLoaded(peerId)
				: nullptr;
			if (peer) {
				appendResolved(peer, username);
			}
		});
	})).fail(crl::guard(this, [=] {
		// Bot not registered yet / unavailable — silently skip its row.
	})).send();
}

void BotsController::appendResolved(
		not_null<PeerData*> peer,
		const QString &username) {
	if (delegate()->peerListFindRow(peer->id.value)) {
		return;
	}
	auto row = std::make_unique<PeerListRow>(peer);
	row->setCustomStatus(u"@"_q + username);
	delegate()->peerListAppendRow(std::move(row));
	delegate()->peerListRefreshRows();
}

void BotsController::rowClicked(not_null<PeerListRow*> row) {
	// Opens the bot chat with the native START button; nothing is auto-sent.
	_window->showPeerHistory(
		row->peer(),
		Window::SectionShow::Way::Forward);
}

class NovagramBotsSection final : public Section<NovagramBotsSection> {
public:
	NovagramBotsSection(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

NovagramBotsSection::NovagramBotsSection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> NovagramBotsSection::title() {
	return tr::lng_fenix_bots();
}

void NovagramBotsSection::setupContent() {
	const auto controller = this->controller();
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	struct CategoryState {
		std::unique_ptr<BotsController> controller;
		std::unique_ptr<PeerListContentDelegateSimple> delegate;
	};

	const auto addCategory = [&](
			rpl::producer<QString> title,
			std::vector<QString> usernames) {
		Ui::AddSkip(content);
		Ui::AddSubsectionTitle(content, std::move(title));

		auto listController = std::make_unique<BotsController>(
			controller,
			std::move(usernames));
		listController->setStyleOverrides(&st::settingsBlockedList);
		const auto raw = listController.get();
		const auto list = content->add(
			object_ptr<PeerListContent>(content, raw));
		const auto state = list->lifetime().make_state<CategoryState>();
		state->controller = std::move(listController);
		state->delegate = std::make_unique<PeerListContentDelegateSimple>();
		state->delegate->setContent(list);
		state->controller->setDelegate(state->delegate.get());
	};

	addCategory(tr::lng_fenix_bots_media(), {
		u"novogram_youtube_bot"_q,
		u"novagram_shazam_bot"_q,
		u"novagram_spotify_bot"_q,
		u"novagram_vk_bot"_q,
		u"novagram_rutube_bot"_q,
		u"novagram_twitch_bot"_q,
		u"novagram_pinterest_bot"_q,
		u"novagram_reddit_bot"_q,
		u"novagram_facebook_bot"_q,
		u"novagram_twitter_bot"_q,
		u"novagram_tiktok_bot"_q,
		u"novagram_instagram_bot"_q,
	});
	addCategory(tr::lng_fenix_bots_editing(), {
		u"novagram_compress_bot"_q,
		u"novagram_crop_bot"_q,
		u"novagram_trimmer_bot"_q,
		u"novagram_blur_bot"_q,
		u"novagram_remover_bot"_q,
		u"novagram_upscaler_bot"_q,
		u"novagram_sticker_bot"_q,
		u"novagram_voice_bot"_q,
	});
	addCategory(tr::lng_fenix_bots_tools(), {
		u"novagram_qr_bot"_q,
		u"novagram_password_bot"_q,
		u"novagram_tempmail_bot"_q,
		u"novagram_pdf_bot"_q,
		u"novagram_tts_bot"_q,
	});
	addCategory(tr::lng_fenix_bots_fun(), {
		u"novagram_meme_bot"_q,
		u"novagram_translit_bot"_q,
		u"novagram_wallpaper_bot"_q,
		u"novagram_movies_bot"_q,
		u"novagram_weather_bot"_q,
		u"novagram_rates_bot"_q,
		u"novagram_facts_bot"_q,
		u"novagram_quotes_bot"_q,
		u"novagram_proverbs_bot"_q,
	});

	Ui::AddSkip(content);
	Ui::ResizeFitChild(this, content);
}

} // namespace

Type NovagramBotsId() {
	return NovagramBotsSection::Id();
}

} // namespace Settings
