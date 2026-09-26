/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Central settings store for Novagram features.
Equivalent of Android GhostVariable/Password singletons — but unified.
Storage: JSON file at cWorkingDir()/tdata/fenix_settings.json
*/
#pragma once

#include <rpl/event_stream.h>

#include <QtCore/QString>

namespace Fenix {

// Default text style applied to the whole message right before sending.
// Mirrors the Android "Text style" dialog. None = off (don't touch the text).
enum class TextStyle {
	None = 0,
	Regular = 1,
	Bold = 2,
	Italic = 3,
	Mono = 4,
	Strike = 5,
	Underline = 6,
	Spoiler = 7,
	Quote = 8,
};

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

	[[nodiscard]] bool hideFolderTabs() const;
	void setHideFolderTabs(bool value);
	[[nodiscard]] rpl::producer<bool> hideFolderTabsChanges() const;
	[[nodiscard]] rpl::producer<bool> hideFolderTabsValue() const;

	[[nodiscard]] bool hideStories() const;
	void setHideStories(bool value);
	[[nodiscard]] rpl::producer<bool> hideStoriesChanges() const;
	[[nodiscard]] rpl::producer<bool> hideStoriesValue() const;

	[[nodiscard]] bool stopAutoDownload() const;
	void setStopAutoDownload(bool value);
	[[nodiscard]] rpl::producer<bool> stopAutoDownloadChanges() const;
	[[nodiscard]] rpl::producer<bool> stopAutoDownloadValue() const;

	// Base64 of the user's native auto-download config, saved before we force
	// it all-off, so toggling the feature back off restores their settings.
	[[nodiscard]] QString autoDownloadBackup() const;
	void setAutoDownloadBackup(const QString &value);

	[[nodiscard]] bool confirmVoice() const;
	void setConfirmVoice(bool value);
	[[nodiscard]] rpl::producer<bool> confirmVoiceChanges() const;
	[[nodiscard]] rpl::producer<bool> confirmVoiceValue() const;

	[[nodiscard]] bool confirmSticker() const;
	void setConfirmSticker(bool value);
	[[nodiscard]] rpl::producer<bool> confirmStickerChanges() const;
	[[nodiscard]] rpl::producer<bool> confirmStickerValue() const;

	[[nodiscard]] bool confirmGif() const;
	void setConfirmGif(bool value);
	[[nodiscard]] rpl::producer<bool> confirmGifChanges() const;
	[[nodiscard]] rpl::producer<bool> confirmGifValue() const;

	[[nodiscard]] TextStyle textStyle() const;
	void setTextStyle(TextStyle value);
	[[nodiscard]] rpl::producer<TextStyle> textStyleChanges() const;
	[[nodiscard]] rpl::producer<TextStyle> textStyleValue() const;

	[[nodiscard]] bool autoAnswer() const;
	void setAutoAnswer(bool value);
	[[nodiscard]] rpl::producer<bool> autoAnswerChanges() const;
	[[nodiscard]] rpl::producer<bool> autoAnswerValue() const;

	[[nodiscard]] QString autoAnswerText() const;
	void setAutoAnswerText(const QString &value);

	// Unread message reminder: if an incoming private message stays unread for
	// [messageReminderDelay] minutes, re-surface its notification once.
	[[nodiscard]] bool messageReminder() const;
	void setMessageReminder(bool value);
	[[nodiscard]] rpl::producer<bool> messageReminderChanges() const;
	[[nodiscard]] rpl::producer<bool> messageReminderValue() const;

	[[nodiscard]] int messageReminderDelay() const;
	void setMessageReminderDelay(int minutes);

	// Approve incoming join requests in chats where you can add members.
	[[nodiscard]] bool autoAcceptJoin() const;
	void setAutoAcceptJoin(bool value);
	[[nodiscard]] rpl::producer<bool> autoAcceptJoinChanges() const;
	[[nodiscard]] rpl::producer<bool> autoAcceptJoinValue() const;

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

	bool _hideFolderTabs = false;
	rpl::event_stream<bool> _hideFolderTabsChanges;

	bool _hideStories = false;
	rpl::event_stream<bool> _hideStoriesChanges;

	bool _stopAutoDownload = false;
	rpl::event_stream<bool> _stopAutoDownloadChanges;
	QString _autoDownloadBackup;

	bool _confirmVoice = false;
	rpl::event_stream<bool> _confirmVoiceChanges;

	bool _confirmSticker = false;
	rpl::event_stream<bool> _confirmStickerChanges;

	bool _confirmGif = false;
	rpl::event_stream<bool> _confirmGifChanges;

	TextStyle _textStyle = TextStyle::None;
	rpl::event_stream<TextStyle> _textStyleChanges;

	bool _autoAnswer = false;
	rpl::event_stream<bool> _autoAnswerChanges;
	QString _autoAnswerText;

	bool _messageReminder = false;
	rpl::event_stream<bool> _messageReminderChanges;
	int _messageReminderDelay = 5;

	bool _autoAcceptJoin = false;
	rpl::event_stream<bool> _autoAcceptJoinChanges;
};

[[nodiscard]] inline Settings &Get() {
	return Settings::Instance();
}

} // namespace Fenix
