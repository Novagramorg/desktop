/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Forwarding kept deleted messages (Android 10dfa598f, 0496cd07b). The server
no longer has them, so a forward of them fails, and with them every other
message of the same request. They go as copies of their text, photo or file
instead, without the "Forwarded from" header the server only writes for a
real forward. See fenix/kept_messages.h.
*/
#pragma once

#include "data/data_types.h"

namespace Api {
struct SendAction;
} // namespace Api

namespace Fenix {

// A kept message the server no longer has, of a kind that can be copied.
[[nodiscard]] bool CanSendKeptCopy(not_null<HistoryItem*> item);

// Sends a copy of a kept message, see CanSendKeptCopy().
void SendKeptCopy(
	not_null<HistoryItem*> item,
	const Api::SendAction &action,
	bool withCaption);

// Takes the messages that go as copies out of a forwarded list, in order.
[[nodiscard]] HistoryItemsList TakeKeptCopies(HistoryItemsList &items);

// Forwards a draft that has kept messages: each goes as a copy in its
// place, the runs of the others between them are forwarded as usual, and
// the callback goes with the last of those. False when it has none.
bool ForwardWithKeptCopies(
	Data::ResolvedForwardDraft &draft,
	const Api::SendAction &action,
	FnMut<void()> &successCallback,
	Fn<void(Data::ResolvedForwardDraft&&, FnMut<void()>&&)> forward);

} // namespace Fenix
