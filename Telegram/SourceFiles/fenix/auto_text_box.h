/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Per-chat editor for the Auto-text signature. Opened from the chat menu; saves a
TextWithTags into Fenix::AutoText for the given peer (empty clears it).
*/
#pragma once

#include "data/data_peer_id.h"

namespace Ui {
class GenericBox;
} // namespace Ui

namespace Fenix {

void FenixAutoTextBox(not_null<Ui::GenericBox*> box, PeerId peerId);

} // namespace Fenix
