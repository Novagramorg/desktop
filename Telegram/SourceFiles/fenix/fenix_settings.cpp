/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/fenix_settings.h"

#include "settings.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <algorithm>

namespace Fenix {
namespace {

[[nodiscard]] QString FilePath() {
	return cWorkingDir() + u"tdata/fenix_settings.json"_q;
}

} // namespace

Settings &Settings::Instance() {
	static Settings instance;
	return instance;
}

Settings::Settings() {
	load();
}

bool Settings::ghostMode() const {
	return _ghostMode;
}

void Settings::setGhostMode(bool value) {
	if (_ghostMode == value) {
		return;
	}
	_ghostMode = value;
	save();
	_ghostModeChanges.fire_copy(value);
}

rpl::producer<bool> Settings::ghostModeChanges() const {
	return _ghostModeChanges.events();
}

rpl::producer<bool> Settings::ghostModeValue() const {
	return rpl::single(_ghostMode) | rpl::then(_ghostModeChanges.events());
}

bool Settings::ghostStory() const {
	return _ghostStory;
}

void Settings::setGhostStory(bool value) {
	if (_ghostStory == value) {
		return;
	}
	_ghostStory = value;
	save();
	_ghostStoryChanges.fire_copy(value);
}

rpl::producer<bool> Settings::ghostStoryChanges() const {
	return _ghostStoryChanges.events();
}

rpl::producer<bool> Settings::ghostStoryValue() const {
	return rpl::single(_ghostStory) | rpl::then(_ghostStoryChanges.events());
}

bool Settings::saveDeletedMsg() const {
	return _saveDeletedMsg;
}

void Settings::setSaveDeletedMsg(bool value) {
	if (_saveDeletedMsg == value) {
		return;
	}
	_saveDeletedMsg = value;
	save();
	_saveDeletedMsgChanges.fire_copy(value);
}

rpl::producer<bool> Settings::saveDeletedMsgChanges() const {
	return _saveDeletedMsgChanges.events();
}

rpl::producer<bool> Settings::saveDeletedMsgValue() const {
	return rpl::single(_saveDeletedMsg) | rpl::then(_saveDeletedMsgChanges.events());
}

bool Settings::saveEditHistory() const {
	return _saveEditHistory;
}

void Settings::setSaveEditHistory(bool value) {
	if (_saveEditHistory == value) {
		return;
	}
	_saveEditHistory = value;
	save();
	_saveEditHistoryChanges.fire_copy(value);
}

rpl::producer<bool> Settings::saveEditHistoryChanges() const {
	return _saveEditHistoryChanges.events();
}

rpl::producer<bool> Settings::saveEditHistoryValue() const {
	return rpl::single(_saveEditHistory) | rpl::then(_saveEditHistoryChanges.events());
}

bool Settings::hideFolderTabs() const {
	return _hideFolderTabs;
}

void Settings::setHideFolderTabs(bool value) {
	if (_hideFolderTabs == value) {
		return;
	}
	_hideFolderTabs = value;
	save();
	_hideFolderTabsChanges.fire_copy(value);
}

rpl::producer<bool> Settings::hideFolderTabsChanges() const {
	return _hideFolderTabsChanges.events();
}

rpl::producer<bool> Settings::hideFolderTabsValue() const {
	return rpl::single(_hideFolderTabs) | rpl::then(_hideFolderTabsChanges.events());
}

bool Settings::hideStories() const {
	return _hideStories;
}

void Settings::setHideStories(bool value) {
	if (_hideStories == value) {
		return;
	}
	_hideStories = value;
	save();
	_hideStoriesChanges.fire_copy(value);
}

rpl::producer<bool> Settings::hideStoriesChanges() const {
	return _hideStoriesChanges.events();
}

rpl::producer<bool> Settings::hideStoriesValue() const {
	return rpl::single(_hideStories) | rpl::then(_hideStoriesChanges.events());
}

bool Settings::stopAutoDownload() const {
	return _stopAutoDownload;
}

void Settings::setStopAutoDownload(bool value) {
	if (_stopAutoDownload == value) {
		return;
	}
	_stopAutoDownload = value;
	save();
	_stopAutoDownloadChanges.fire_copy(value);
}

rpl::producer<bool> Settings::stopAutoDownloadChanges() const {
	return _stopAutoDownloadChanges.events();
}

rpl::producer<bool> Settings::stopAutoDownloadValue() const {
	return rpl::single(_stopAutoDownload) | rpl::then(_stopAutoDownloadChanges.events());
}

QString Settings::autoDownloadBackup() const {
	return _autoDownloadBackup;
}

void Settings::setAutoDownloadBackup(const QString &value) {
	if (_autoDownloadBackup == value) {
		return;
	}
	_autoDownloadBackup = value;
	save();
}

bool Settings::confirmVoice() const {
	return _confirmVoice;
}

void Settings::setConfirmVoice(bool value) {
	if (_confirmVoice == value) {
		return;
	}
	_confirmVoice = value;
	save();
	_confirmVoiceChanges.fire_copy(value);
}

rpl::producer<bool> Settings::confirmVoiceChanges() const {
	return _confirmVoiceChanges.events();
}

rpl::producer<bool> Settings::confirmVoiceValue() const {
	return rpl::single(_confirmVoice) | rpl::then(_confirmVoiceChanges.events());
}

bool Settings::confirmSticker() const {
	return _confirmSticker;
}

void Settings::setConfirmSticker(bool value) {
	if (_confirmSticker == value) {
		return;
	}
	_confirmSticker = value;
	save();
	_confirmStickerChanges.fire_copy(value);
}

rpl::producer<bool> Settings::confirmStickerChanges() const {
	return _confirmStickerChanges.events();
}

rpl::producer<bool> Settings::confirmStickerValue() const {
	return rpl::single(_confirmSticker) | rpl::then(_confirmStickerChanges.events());
}

bool Settings::confirmGif() const {
	return _confirmGif;
}

void Settings::setConfirmGif(bool value) {
	if (_confirmGif == value) {
		return;
	}
	_confirmGif = value;
	save();
	_confirmGifChanges.fire_copy(value);
}

rpl::producer<bool> Settings::confirmGifChanges() const {
	return _confirmGifChanges.events();
}

rpl::producer<bool> Settings::confirmGifValue() const {
	return rpl::single(_confirmGif) | rpl::then(_confirmGifChanges.events());
}

TextStyle Settings::textStyle() const {
	return _textStyle;
}

void Settings::setTextStyle(TextStyle value) {
	if (_textStyle == value) {
		return;
	}
	_textStyle = value;
	save();
	_textStyleChanges.fire_copy(value);
}

rpl::producer<TextStyle> Settings::textStyleChanges() const {
	return _textStyleChanges.events();
}

rpl::producer<TextStyle> Settings::textStyleValue() const {
	return rpl::single(_textStyle) | rpl::then(_textStyleChanges.events());
}

bool Settings::autoAnswer() const {
	return _autoAnswer;
}

void Settings::setAutoAnswer(bool value) {
	if (_autoAnswer == value) {
		return;
	}
	_autoAnswer = value;
	save();
	_autoAnswerChanges.fire_copy(value);
}

rpl::producer<bool> Settings::autoAnswerChanges() const {
	return _autoAnswerChanges.events();
}

rpl::producer<bool> Settings::autoAnswerValue() const {
	return rpl::single(_autoAnswer) | rpl::then(_autoAnswerChanges.events());
}

QString Settings::autoAnswerText() const {
	return _autoAnswerText;
}

void Settings::setAutoAnswerText(const QString &value) {
	if (_autoAnswerText == value) {
		return;
	}
	_autoAnswerText = value;
	save();
}

bool Settings::messageReminder() const {
	return _messageReminder;
}

void Settings::setMessageReminder(bool value) {
	if (_messageReminder == value) {
		return;
	}
	_messageReminder = value;
	save();
	_messageReminderChanges.fire_copy(value);
}

rpl::producer<bool> Settings::messageReminderChanges() const {
	return _messageReminderChanges.events();
}

rpl::producer<bool> Settings::messageReminderValue() const {
	return rpl::single(_messageReminder)
		| rpl::then(_messageReminderChanges.events());
}

int Settings::messageReminderDelay() const {
	return _messageReminderDelay;
}

void Settings::setMessageReminderDelay(int minutes) {
	const auto clamped = std::clamp(minutes, 1, 59);
	if (_messageReminderDelay == clamped) {
		return;
	}
	_messageReminderDelay = clamped;
	save();
}

bool Settings::autoAcceptJoin() const {
	return _autoAcceptJoin;
}

void Settings::setAutoAcceptJoin(bool value) {
	if (_autoAcceptJoin == value) {
		return;
	}
	_autoAcceptJoin = value;
	save();
	_autoAcceptJoinChanges.fire_copy(value);
}

rpl::producer<bool> Settings::autoAcceptJoinChanges() const {
	return _autoAcceptJoinChanges.events();
}

rpl::producer<bool> Settings::autoAcceptJoinValue() const {
	return rpl::single(_autoAcceptJoin)
		| rpl::then(_autoAcceptJoinChanges.events());
}

bool Settings::avatarOpensProfile() const {
	return _avatarOpensProfile;
}

void Settings::setAvatarOpensProfile(bool value) {
	if (_avatarOpensProfile == value) {
		return;
	}
	_avatarOpensProfile = value;
	save();
	_avatarOpensProfileChanges.fire_copy(value);
}

rpl::producer<bool> Settings::avatarOpensProfileChanges() const {
	return _avatarOpensProfileChanges.events();
}

rpl::producer<bool> Settings::avatarOpensProfileValue() const {
	return rpl::single(_avatarOpensProfile)
		| rpl::then(_avatarOpensProfileChanges.events());
}

bool Settings::showSeconds() const {
	return _showSeconds;
}

void Settings::setShowSeconds(bool value) {
	if (_showSeconds == value) {
		return;
	}
	_showSeconds = value;
	save();
	_showSecondsChanges.fire_copy(value);
}

rpl::producer<bool> Settings::showSecondsChanges() const {
	return _showSecondsChanges.events();
}

rpl::producer<bool> Settings::showSecondsValue() const {
	return rpl::single(_showSeconds)
		| rpl::then(_showSecondsChanges.events());
}

void Settings::load() {
	auto file = QFile(FilePath());
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto bytes = file.readAll();
	file.close();

	auto error = QJsonParseError();
	const auto doc = QJsonDocument::fromJson(bytes, &error);
	if (error.error != QJsonParseError::NoError || !doc.isObject()) {
		return;
	}
	const auto root = doc.object();
	_ghostMode = root.value(u"ghost_mode"_q).toBool(false);
	_ghostStory = root.value(u"ghost_story"_q).toBool(false);
	_saveDeletedMsg = root.value(u"save_deleted_msg"_q).toBool(false);
	_saveEditHistory = root.value(u"save_edit_history"_q).toBool(false);
	_hideFolderTabs = root.value(u"hide_folder_tabs"_q).toBool(false);
	_hideStories = root.value(u"hide_stories"_q).toBool(false);
	_stopAutoDownload = root.value(u"stop_auto_download"_q).toBool(false);
	_autoDownloadBackup = root.value(u"auto_download_backup"_q).toString();
	_confirmVoice = root.value(u"confirm_voice"_q).toBool(false);
	_confirmSticker = root.value(u"confirm_sticker"_q).toBool(false);
	_confirmGif = root.value(u"confirm_gif"_q).toBool(false);
	const auto style = root.value(u"text_style"_q).toInt(0);
	_textStyle = (style >= int(TextStyle::None) && style <= int(TextStyle::Quote))
		? static_cast<TextStyle>(style)
		: TextStyle::None;
	_autoAnswer = root.value(u"auto_answer"_q).toBool(false);
	_autoAnswerText = root.value(u"auto_answer_text"_q).toString();
	_messageReminder = root.value(u"message_reminder"_q).toBool(false);
	_messageReminderDelay = std::clamp(
		root.value(u"message_reminder_delay"_q).toInt(5),
		1,
		59);
	_autoAcceptJoin = root.value(u"auto_accept_join"_q).toBool(false);
	_avatarOpensProfile = root.value(u"avatar_opens_profile"_q).toBool(false);
	_showSeconds = root.value(u"show_seconds"_q).toBool(false);
}

void Settings::save() {
	auto root = QJsonObject();
	root.insert(u"ghost_mode"_q, _ghostMode);
	root.insert(u"ghost_story"_q, _ghostStory);
	root.insert(u"save_deleted_msg"_q, _saveDeletedMsg);
	root.insert(u"save_edit_history"_q, _saveEditHistory);
	root.insert(u"hide_folder_tabs"_q, _hideFolderTabs);
	root.insert(u"hide_stories"_q, _hideStories);
	root.insert(u"stop_auto_download"_q, _stopAutoDownload);
	root.insert(u"auto_download_backup"_q, _autoDownloadBackup);
	root.insert(u"confirm_voice"_q, _confirmVoice);
	root.insert(u"confirm_sticker"_q, _confirmSticker);
	root.insert(u"confirm_gif"_q, _confirmGif);
	root.insert(u"text_style"_q, int(_textStyle));
	root.insert(u"auto_answer"_q, _autoAnswer);
	root.insert(u"auto_answer_text"_q, _autoAnswerText);
	root.insert(u"message_reminder"_q, _messageReminder);
	root.insert(u"message_reminder_delay"_q, _messageReminderDelay);
	root.insert(u"auto_accept_join"_q, _autoAcceptJoin);
	root.insert(u"avatar_opens_profile"_q, _avatarOpensProfile);
	root.insert(u"show_seconds"_q, _showSeconds);

	const auto path = FilePath();
	QDir().mkpath(QFileInfo(path).absolutePath());

	auto file = QFile(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return;
	}
	file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	file.close();
}

} // namespace Fenix
