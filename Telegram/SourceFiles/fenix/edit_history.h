/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Keeps the previous text of edited messages so the user can review the edit
history of a message. Equivalent of Android org.fenixuz.utils.EditMessage.
One store per account, see fenix/saved_history.h.
*/
#pragma once

#include "base/timer.h"
#include "fenix/saved_history.h"

#include <QtCore/QString>

#include <map>
#include <vector>

namespace Fenix {

class EditHistory final {
public:
	struct Version {
		QString text;
		TimeId date = 0;
	};

	explicit EditHistory(not_null<Main::Session*> session);
	~EditHistory();

	// Records the text a message held before it was edited. The newest
	// version is appended last. Consecutive duplicates are ignored.
	void recordEdit(PeerId peerId, MsgId msgId, QString text, TimeId date);
	[[nodiscard]] std::vector<Version> versions(
		PeerId peerId,
		MsgId msgId) const;
	[[nodiscard]] bool has(PeerId peerId, MsgId msgId) const;
	void forget(PeerId peerId, MsgId msgId);
	void forgetChat(PeerId peerId);
	void clear();

	// Versions of the old file shared by all accounts.
	void importLegacy(
		std::vector<std::pair<FullMsgId, std::vector<Version>>> list);

	// The account logs out: nothing is kept or written any more.
	void wipe();

private:
	void load();
	void save();
	void saveDelayed();
	void trim();

	const SessionFile _file;
	std::map<FullMsgId, std::vector<Version>> _records;
	int64 _textSize = 0;
	base::Timer _saveTimer;
	bool _wiped = false;

};

[[nodiscard]] EditHistory &GetEdits(not_null<Main::Session*> session);

} // namespace Fenix
