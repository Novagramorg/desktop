/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/fenix_settings.h"

#include "settings.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

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

bool Settings::storyDownload() const {
	return _storyDownload;
}

void Settings::setStoryDownload(bool value) {
	if (_storyDownload == value) {
		return;
	}
	_storyDownload = value;
	save();
	_storyDownloadChanges.fire_copy(value);
}

rpl::producer<bool> Settings::storyDownloadChanges() const {
	return _storyDownloadChanges.events();
}

rpl::producer<bool> Settings::storyDownloadValue() const {
	return rpl::single(_storyDownload) | rpl::then(_storyDownloadChanges.events());
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
	_saveDeletedMsg = root.value(u"save_deleted_msg"_q).toBool(false);
	_saveEditHistory = root.value(u"save_edit_history"_q).toBool(false);
	_storyDownload = root.value(u"story_download"_q).toBool(false);
}

void Settings::save() {
	auto root = QJsonObject();
	root.insert(u"ghost_mode"_q, _ghostMode);
	root.insert(u"save_deleted_msg"_q, _saveDeletedMsg);
	root.insert(u"save_edit_history"_q, _saveEditHistory);
	root.insert(u"story_download"_q, _storyDownload);

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
