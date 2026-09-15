/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Per-chat "One-time voice" preference. When enabled for a chat, every new voice /
round-video recording in that chat defaults to Telegram's native view-once mode
(the record bar's TTL "once" button starts active), so the recipient can listen
or watch it only once. The user can still flip a single recording with the native
once button — this only changes the DEFAULT. Equivalent of the Android
org.fenixuz.utils.OneTimeVoice engine.

Storage: JSON file at cWorkingDir()/tdata/fenix_onetimevoice.json
*/
#pragma once

#include "data/data_peer_id.h"

#include <rpl/event_stream.h>

#include <map>

namespace Fenix {

class OneTimeVoice final {
public:
	[[nodiscard]] static OneTimeVoice &Instance();

	[[nodiscard]] bool isEnabled(PeerId peerId) const;
	void setEnabled(PeerId peerId, bool enabled);
	bool toggle(PeerId peerId);

	[[nodiscard]] rpl::producer<> changes() const;

private:
	OneTimeVoice();
	~OneTimeVoice() = default;

	void load();
	void save();

	std::map<PeerId, bool> _enabled;
	rpl::event_stream<> _changes;
};

[[nodiscard]] inline OneTimeVoice &GetOneTimeVoice() {
	return OneTimeVoice::Instance();
}

} // namespace Fenix
