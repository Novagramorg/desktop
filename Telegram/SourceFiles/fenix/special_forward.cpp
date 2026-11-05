/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/special_forward.h"

#include "history/view/history_view_list_widget.h"
#include "window/window_peer_menu.h"
#include "window/window_session_controller.h"

namespace Fenix {

void SpecialForwardSelected(not_null<HistoryView::ListWidget*> widget) {
	const auto items = widget->getSelectedItems();
	if (items.empty()
		|| !ranges::all_of(items, &HistoryView::SelectedItem::canForward)) {
		return;
	}
	const auto weak = base::make_weak(widget);
	Window::ShowForwardMessagesBox(
		widget->controller(),
		Data::ForwardDraft{
			.ids = widget->getSelectedIds(),
			.options = Data::ForwardOptions::NoSenderNames,
		},
		[=] {
			if (const auto strong = weak.get()) {
				strong->cancelSelection();
			}
		});
}

} // namespace Fenix
