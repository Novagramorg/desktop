/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Per-chat "Auto-text" signature appended to every outgoing text message in that
chat. Stored as a TextWithTags (text + basic formatting), so the signature can
itself be styled. Equivalent of the Android org.fenixuz.utils.AutoTextAppender
(storage half) — the send-path append lives in HistoryWidget and the editor box
in fenix/auto_text_box.

Storage: JSON file at cWorkingDir()/tdata/fenix_autotext.json
*/
#pragma once

#include "data/data_peer_id.h"
#include "ui/text/text_entity.h"

#include <rpl/event_stream.h>

#include <map>

namespace Fenix {

class AutoText final {
public:
	[[nodiscard]] static AutoText &Instance();

	[[nodiscard]] TextWithTags get(PeerId peerId) const;
	[[nodiscard]] bool isActive(PeerId peerId) const;
	void set(PeerId peerId, TextWithTags value);

	[[nodiscard]] rpl::producer<> changes() const;

private:
	AutoText();
	~AutoText() = default;

	void load();
	void save();

	std::map<PeerId, TextWithTags> _entries;
	rpl::event_stream<> _changes;
};

[[nodiscard]] inline AutoText &GetAutoText() {
	return AutoText::Instance();
}

} // namespace Fenix
