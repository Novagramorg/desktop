/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/auto_accept_join.h"

#include "fenix/fenix_settings.h"
#include "apiwrap.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_peer.h"
#include "main/main_session.h"

namespace Fenix {
namespace {

constexpr auto kRetryAfter = crl::time(60000);

} // namespace

AutoAcceptJoin &AutoAcceptJoin::Instance() {
	static AutoAcceptJoin instance;
	return instance;
}

void AutoAcceptJoin::process(not_null<PeerData*> peer, int pendingCount) {
	// Approving fires another update for the same peer with no requests left,
	// so the count check is what keeps this from looping. The in-flight map
	// covers the window before that update arrives; its timestamps let a peer
	// recover if the reply never comes (a logout cancels pending requests
	// without invoking either callback).
	if (!Get().autoAcceptJoin() || pendingCount <= 0) {
		return;
	}
	const auto chat = peer->asChat();
	const auto channel = peer->asChannel();
	const auto allowed = chat
		? chat->canAddMembers()
		: (channel && channel->canAddMembers());
	if (!allowed) {
		return;
	}
	const auto peerId = peer->id;
	const auto now = crl::now();
	const auto i = _sending.find(peerId);
	if (i != _sending.end() && (now - i->second < kRetryAfter)) {
		return;
	}
	_sending[peerId] = now;

	const auto session = &peer->session();
	using Flag = MTPmessages_HideAllChatJoinRequests::Flag;
	session->api().request(MTPmessages_HideAllChatJoinRequests(
		MTP_flags(Flag::f_approved),
		peer->input(),
		MTPstring()
	)).done([=](const MTPUpdates &result) {
		Instance().finish(peerId);
		session->api().applyUpdates(result);
	}).fail([=](const MTP::Error &) {
		Instance().finish(peerId);
	}).send();
}

void AutoAcceptJoin::finish(PeerId peerId) {
	_sending.remove(peerId);
}

} // namespace Fenix
