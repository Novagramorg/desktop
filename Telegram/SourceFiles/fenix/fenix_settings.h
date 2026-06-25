/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.

Central settings store for FenixUz features.
Equivalent of Android GhostVariable/Password singletons — but unified.
Storage: JSON file at cWorkingDir()/tdata/fenix_settings.json
*/
#pragma once

#include <rpl/event_stream.h>

namespace Fenix {

class Settings final {
public:
	[[nodiscard]] static Settings &Instance();

	[[nodiscard]] bool ghostMode() const;
	void setGhostMode(bool value);
	[[nodiscard]] rpl::producer<bool> ghostModeChanges() const;
	[[nodiscard]] rpl::producer<bool> ghostModeValue() const;

	[[nodiscard]] bool saveDeletedMsg() const;
	void setSaveDeletedMsg(bool value);
	[[nodiscard]] rpl::producer<bool> saveDeletedMsgChanges() const;
	[[nodiscard]] rpl::producer<bool> saveDeletedMsgValue() const;

	[[nodiscard]] bool saveEditHistory() const;
	void setSaveEditHistory(bool value);
	[[nodiscard]] rpl::producer<bool> saveEditHistoryChanges() const;
	[[nodiscard]] rpl::producer<bool> saveEditHistoryValue() const;

	[[nodiscard]] bool storyDownload() const;
	void setStoryDownload(bool value);
	[[nodiscard]] rpl::producer<bool> storyDownloadChanges() const;
	[[nodiscard]] rpl::producer<bool> storyDownloadValue() const;

private:
	Settings();
	~Settings() = default;

	void load();
	void save();

	bool _ghostMode = false;
	rpl::event_stream<bool> _ghostModeChanges;

	bool _saveDeletedMsg = false;
	rpl::event_stream<bool> _saveDeletedMsgChanges;

	bool _saveEditHistory = false;
	rpl::event_stream<bool> _saveEditHistoryChanges;

	bool _storyDownload = false;
	rpl::event_stream<bool> _storyDownloadChanges;
};

[[nodiscard]] inline Settings &Get() {
	return Settings::Instance();
}

} // namespace Fenix
