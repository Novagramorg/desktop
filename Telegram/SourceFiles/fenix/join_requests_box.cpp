/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/join_requests_box.h"

#include "apiwrap.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_peer.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "ui/layers/generic_box.h"
#include "ui/layers/show.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"

namespace Fenix {
namespace {

[[nodiscard]] int PendingCount(not_null<PeerData*> peer) {
	if (const auto channel = peer->asChannel()) {
		return channel->pendingRequestsCount();
	} else if (const auto chat = peer->asChat()) {
		return chat->pendingRequestsCount();
	}
	return 0;
}

} // namespace

void JoinRequestsBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	box->setTitle(tr::lng_fenix_join_requests());

	const auto count = PendingCount(peer);
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		(count > 0
			? tr::lng_fenix_join_requests_count(tr::now, lt_count, count)
			: tr::lng_fenix_join_requests_none(tr::now)),
		st::boxLabel));

	if (count <= 0) {
		box->addButton(tr::lng_close(), [=] { box->closeBox(); });
		return;
	}

	const auto session = &controller->session();
	const auto show = controller->uiShow();
	const auto run = [=](bool approved) {
		using Flag = MTPmessages_HideAllChatJoinRequests::Flag;
		session->api().request(MTPmessages_HideAllChatJoinRequests(
			MTP_flags(approved ? Flag::f_approved : Flag(0)),
			peer->input(),
			MTPstring()
		)).done([=](const MTPUpdates &result) {
			session->api().applyUpdates(result);
			show->showToast(approved
				? tr::lng_fenix_join_requests_approved(tr::now)
				: tr::lng_fenix_join_requests_declined(tr::now));
		}).fail([=](const MTP::Error &error) {
			show->showToast(tr::lng_fenix_join_requests_error(tr::now));
		}).send();
		box->closeBox();
	};

	box->addButton(tr::lng_fenix_join_requests_approve(), [=] { run(true); });
	box->addLeftButton(
		tr::lng_fenix_join_requests_decline(),
		[=] { run(false); });
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

} // namespace Fenix
