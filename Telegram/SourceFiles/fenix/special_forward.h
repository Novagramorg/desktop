/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#pragma once

namespace HistoryView {
class ListWidget;
} // namespace HistoryView

namespace Fenix {

// Novagram "Special forward" of the messages selected in a list: the same
// as Forward, with the senders' names hidden (Android hideSpetialFunction).
void SpecialForwardSelected(not_null<HistoryView::ListWidget*> widget);

} // namespace Fenix
