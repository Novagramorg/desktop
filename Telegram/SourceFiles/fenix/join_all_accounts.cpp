/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/join_all_accounts.h"

#include "apiwrap.h"
#include "core/application.h"
#include "data/data_channel.h"
#include "data/data_session.h"
#include "lang/lang_keys.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "ui/layers/show.h"
#include "window/window_session_controller.h"

#include <memory>
#include <vector>

namespace Fenix {
namespace {

struct JoinState {
	int total = 0;
	int done = 0;
	int joined = 0;
};

// Resolve the username on this account (separate access hash) and join.
void JoinOne(
		not_null<Main::Session*> session,
		const QString &username,
		Fn<void(bool)> finished) {
	session->api().request(MTPcontacts_ResolveUsername(
		MTP_flags(0),
		MTP_string(username),
		MTP_string()
	)).done([=](const MTPcontacts_ResolvedPeer &result) {
		result.match([&](const MTPDcontacts_resolvedPeer &data) {
			session->data().processUsers(data.vusers());
			session->data().processChats(data.vchats());
			const auto peerId = peerFromMTP(data.vpeer());
			const auto peer = peerId
				? session->data().peerLoaded(peerId)
				: nullptr;
			const auto channel = peer ? peer->asChannel() : nullptr;
			if (!channel) {
				finished(false);
				return;
			} else if (channel->amIn()) {
				finished(true);
				return;
			}
			session->api().request(MTPchannels_JoinChannel(
				channel->inputChannel()
			)).done([=](const MTPUpdates &updates) {
				session->api().applyUpdates(updates);
				finished(true);
			}).fail([=](const MTP::Error &error) {
				finished(error.type() == u"USER_ALREADY_PARTICIPANT"_q);
			}).send();
		});
	}).fail([=](const MTP::Error &error) {
		finished(false);
	}).send();
}

} // namespace

void JoinChannelOnAllAccounts(
		not_null<Window::SessionController*> controller,
		not_null<ChannelData*> channel) {
	const auto username = channel->username();
	if (username.isEmpty()) {
		return;
	}
	const auto current = &controller->session();
	auto sessions = std::vector<not_null<Main::Session*>>();
	for (const auto &[index, account] : Core::App().domain().accounts()) {
		if (const auto session = account->maybeSession()) {
			if (session != current) {
				sessions.push_back(session);
			}
		}
	}
	const auto show = controller->uiShow();
	if (sessions.empty()) {
		show->showToast(tr::lng_fenix_join_all_none(tr::now));
		return;
	}
	show->showToast(tr::lng_fenix_join_all_progress(tr::now));

	const auto state = std::make_shared<JoinState>();
	state->total = int(sessions.size());
	for (const auto &session : sessions) {
		JoinOne(session, username, [=](bool ok) {
			++state->done;
			if (ok) {
				++state->joined;
			}
			if (state->done == state->total) {
				show->showToast(tr::lng_fenix_join_all_done(
					tr::now,
					lt_count,
					state->joined));
			}
		});
	}
}

} // namespace Fenix
