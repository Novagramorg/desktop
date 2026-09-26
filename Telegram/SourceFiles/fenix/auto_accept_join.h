/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Auto-accept join requests. When the server reports pending join requests for a
channel or group where the user may add members, approve them all in one call.
Equivalent of the Android org.fenixuz.utils.AutoAcceptJoin object.
*/
#pragma once

#include "base/flat_map.h"
#include "base/timer.h"
#include "data/data_peer_id.h"

class PeerData;

namespace Fenix {

class AutoAcceptJoin final {
public:
	[[nodiscard]] static AutoAcceptJoin &Instance();

	void process(not_null<PeerData*> peer, int pendingCount);

private:
	AutoAcceptJoin() = default;
	~AutoAcceptJoin() = default;

	void finish(PeerId peerId);

	base::flat_map<PeerId, crl::time> _sending;

};

[[nodiscard]] inline AutoAcceptJoin &GetAutoAcceptJoin() {
	return AutoAcceptJoin::Instance();
}

} // namespace Fenix
