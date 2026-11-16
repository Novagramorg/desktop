/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/join_all_accounts.h"

#include "apiwrap.h"
#include "base/call_delayed.h"
#include "base/random.h"
#include "base/weak_ptr.h"
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

// The accounts join one at a time, a random 2-5 s apart. All of them joining
// the same channel in the same second, from one device and one IP, is the
// mass join Telegram's anti-spam looks for, and accounts were frozen for it.
// The pause is random so the joins don't come at a fixed period either.
constexpr auto kMinGap = crl::time(2000);
constexpr auto kMaxGap = crl::time(5000);

enum class JoinResult {
	Joined,
	Failed,
	Flood,
};

struct JoinRun {
	QString username;
	std::vector<base::weak_ptr<Main::Session>> sessions;
	std::shared_ptr<Ui::Show> show;
	int index = 0;
	int joined = 0;
};

// The run in progress. Its requests and its pause hold it: when an account
// logs out in the middle of its request, the request is dropped with the run.
std::weak_ptr<JoinRun> CurrentRun;

// Resolve the username on this account (separate access hash) and join.
// A FLOOD_WAIT comes back as Flood, not waited out and sent again.
void JoinOne(
		not_null<Main::Session*> session,
		const QString &username,
		Fn<void(JoinResult)> finished) {
	const auto failed = [=](const MTP::Error &error) {
		finished(MTP::IsFloodError(error)
			? JoinResult::Flood
			: JoinResult::Failed);
	};
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
				finished(JoinResult::Failed);
				return;
			} else if (channel->amIn()) {
				finished(JoinResult::Joined);
				return;
			}
			session->api().request(MTPchannels_JoinChannel(
				channel->inputChannel()
			)).done([=](const MTPmessages_ChatInviteJoinResult &result) {
				result.match([&](const MTPDmessages_chatInviteJoinResultOk &data) {
					session->api().applyUpdates(data.vupdates());
					finished(JoinResult::Joined);
				}, [&](const MTPDmessages_chatInviteJoinResultWebView &) {
					finished(JoinResult::Failed);
				});
			}).fail([=](const MTP::Error &error) {
				if (error.type() == u"USER_ALREADY_PARTICIPANT"_q) {
					finished(JoinResult::Joined);
				} else {
					failed(error);
				}
			}).handleFloodErrors().send();
		});
	}).fail(failed).handleFloodErrors().send();
}

void Finish(const std::shared_ptr<JoinRun> &run, bool flood) {
	CurrentRun.reset();
	auto text = tr::lng_fenix_join_all_done(
		tr::now,
		lt_count,
		run->joined);
	if (flood) {
		// The rest would only add to the limit the server has set.
		text += u"\n"_q + tr::lng_flood_error(tr::now);
	}
	run->show->showToast(text);
}

void JoinNext(const std::shared_ptr<JoinRun> &run) {
	const auto count = int(run->sessions.size());
	while (run->index < count && !run->sessions[run->index].get()) {
		++run->index; // Logged out since the run started.
	}
	if (run->index >= count) {
		Finish(run, false);
		return;
	}
	const auto session = run->sessions[run->index].get();
	JoinOne(session, run->username, [=](JoinResult result) {
		if (result == JoinResult::Joined) {
			++run->joined;
		}
		++run->index;
		if (result == JoinResult::Flood) {
			Finish(run, true);
		} else if (run->index >= count) {
			Finish(run, false);
		} else {
			const auto gap = kMinGap
				+ base::RandomIndex(int(kMaxGap - kMinGap) + 1);
			base::call_delayed(gap, [=] {
				JoinNext(run);
			});
		}
	});
}

} // namespace

void JoinChannelOnAllAccounts(
		not_null<Window::SessionController*> controller,
		not_null<ChannelData*> channel) {
	const auto username = channel->username();
	if (username.isEmpty()) {
		return;
	}
	const auto show = controller->uiShow();
	if (CurrentRun.lock()) {
		// One run at a time: a second one would join alongside it.
		show->showToast(tr::lng_fenix_join_all_progress(tr::now));
		return;
	}
	const auto current = &controller->session();
	auto sessions = std::vector<base::weak_ptr<Main::Session>>();

	// All accounts means this one too when it isn't in the channel yet. It
	// goes first, so the user sees it join at once.
	if (!channel->amIn()) {
		sessions.push_back(base::make_weak(current));
	}
	for (const auto &[index, account] : Core::App().domain().accounts()) {
		if (const auto session = account->maybeSession()) {
			if (session != current) {
				sessions.push_back(base::make_weak(session));
			}
		}
	}
	if (sessions.empty()) {
		show->showToast(tr::lng_fenix_join_all_none(tr::now));
		return;
	}
	show->showToast(tr::lng_fenix_join_all_progress(tr::now));

	const auto run = std::make_shared<JoinRun>(JoinRun{
		.username = username,
		.sessions = std::move(sessions),
		.show = show,
	});
	CurrentRun = run;
	JoinNext(run);
}

} // namespace Fenix
