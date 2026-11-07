/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/kept_forward.h"

#include "api/api_common.h"
#include "api/api_sending.h"
#include "apiwrap.h"
#include "data/data_media_types.h"
#include "fenix/deleted_msg.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "ui/text/text_entity.h"

namespace Fenix {
namespace {

// Kept after someone deleted it: the server no longer has it.
[[nodiscard]] bool IsKept(not_null<HistoryItem*> item) {
	const auto history = item->history();
	return IsServerMsgId(item->id)
		&& GetDeleted(&history->session()).whoDeleted(
			history->peer->id,
			item->id).has_value();
}

} // namespace

bool CanSendKeptCopy(not_null<HistoryItem*> item) {
	if (!IsKept(item)) {
		return false; // The server has it: a real forward.
	} else if (!item->allowsForward()) {
		return false; // Protected, or a self-destructing photo or video.
	}
	const auto media = item->media();
	if (!media || media->webpage()) {
		return !item->originalText().empty();
	}
	return media->photo() || media->document();
}

void SendKeptCopy(
		not_null<HistoryItem*> item,
		const Api::SendAction &action,
		bool withCaption) {
	auto message = Api::MessageToSend(action);
	message.action.clearDraft = false;

	const auto media = item->media();
	const auto textOnly = !media || media->webpage();
	if (textOnly || withCaption) {
		const auto &text = item->originalText();
		message.textWithTags = TextWithTags{
			text.text,
			TextUtilities::ConvertEntitiesToTextTags(text.entities),
		};
	}
	if (textOnly) {
		item->history()->session().api().sendMessage(std::move(message));
	} else if (const auto photo = media->photo()) {
		Api::SendExistingPhoto(std::move(message), photo);
	} else if (const auto document = media->document()) {
		Api::SendExistingDocument(std::move(message), document);
	}
}

HistoryItemsList TakeKeptCopies(HistoryItemsList &items) {
	auto result = HistoryItemsList();
	for (auto i = begin(items); i != end(items);) {
		if (CanSendKeptCopy(*i)) {
			result.push_back(*i);
			i = items.erase(i);
		} else {
			++i;
		}
	}
	ranges::sort(result, ranges::less(), &HistoryItem::position);
	return result;
}

bool ForwardWithKeptCopies(
		Data::ResolvedForwardDraft &draft,
		const Api::SendAction &action,
		FnMut<void()> &successCallback,
		Fn<void(Data::ResolvedForwardDraft&&, FnMut<void()>&&)> forward) {
	if (ranges::none_of(draft.items, CanSendKeptCopy)) {
		return false;
	}
	auto ordered = draft.items;
	ranges::sort(ordered, ranges::less(), &HistoryItem::position);

	// In order: runs of messages to forward, and single copies between.
	// A kept one that can't be copied is forwarded alone, so it fails
	// alone. No part has a copy left, so forward() never comes back here.
	struct Part {
		HistoryItemsList forward;
		HistoryItem *copy = nullptr;
		bool alone = false;
	};
	auto parts = std::vector<Part>();
	for (const auto &item : ordered) {
		if (CanSendKeptCopy(item)) {
			parts.push_back({ .copy = item });
		} else if (IsKept(item)) {
			parts.push_back({ .forward = { item }, .alone = true });
		} else {
			if (parts.empty() || parts.back().copy || parts.back().alone) {
				parts.push_back({});
			}
			parts.back().forward.push_back(item);
		}
	}
	auto lastForward = -1;
	for (auto i = 0, count = int(parts.size()); i != count; ++i) {
		if (!parts[i].copy) {
			lastForward = i;
		}
	}
	const auto withCaption = (draft.options
		!= Data::ForwardOptions::NoNamesAndCaptions);

	// Sent one after another (see Histories::sendPreparedMessage), so
	// they come in the order of the messages.
	for (auto i = 0, count = int(parts.size()); i != count; ++i) {
		auto &part = parts[i];
		if (const auto item = part.copy) {
			SendKeptCopy(item, action, withCaption);
		} else {
			forward(
				Data::ResolvedForwardDraft{
					.items = std::move(part.forward),
					.options = draft.options,
				},
				(i == lastForward)
					? base::take(successCallback)
					: FnMut<void()>());
		}
	}
	if (successCallback) {
		// Only copies: they are sent.
		base::take(successCallback)();
	}
	return true;
}

} // namespace Fenix
