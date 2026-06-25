/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.

Keeps the previous text of edited messages so the user can review the edit
history of a message. Equivalent of Android org.fenixuz.utils.EditMessage.
Storage: JSON file at cWorkingDir()/tdata/fenix_edits.json
*/
#pragma once

#include <rpl/event_stream.h>

#include <QtCore/QString>

#include <map>
#include <vector>

namespace Fenix {

class EditHistory final {
public:
	[[nodiscard]] static EditHistory &Instance();

	struct Version {
		QString text;
		TimeId date = 0;
	};

	// Records the text a message held before it was edited. The newest
	// version is appended last. Consecutive duplicates are ignored.
	void recordEdit(PeerId peerId, MsgId msgId, QString text, TimeId date);
	[[nodiscard]] std::vector<Version> versions(
		PeerId peerId,
		MsgId msgId) const;
	[[nodiscard]] bool has(PeerId peerId, MsgId msgId) const;
	void forget(PeerId peerId, MsgId msgId);
	void forgetAll(PeerId peerId);

	[[nodiscard]] rpl::producer<> changes() const;

private:
	EditHistory();
	~EditHistory() = default;

	void load();
	void save();
	void scheduleSave();

	struct Key {
		PeerId peer;
		MsgId msg;
		friend constexpr bool operator<(
				const Key &a, const Key &b) noexcept {
			return std::tie(a.peer, a.msg) < std::tie(b.peer, b.msg);
		}
		friend constexpr bool operator==(
				const Key &a, const Key &b) noexcept = default;
	};
	std::map<Key, std::vector<Version>> _records;
	rpl::event_stream<> _changes;
};

[[nodiscard]] inline EditHistory &GetEdits() {
	return EditHistory::Instance();
}

} // namespace Fenix
