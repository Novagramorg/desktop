/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/one_time_voice.h"

#include "settings.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

namespace Fenix {
namespace {

[[nodiscard]] QString FilePath() {
	return cWorkingDir() + u"tdata/fenix_onetimevoice.json"_q;
}

} // namespace

OneTimeVoice &OneTimeVoice::Instance() {
	static OneTimeVoice instance;
	return instance;
}

OneTimeVoice::OneTimeVoice() {
	load();
}

bool OneTimeVoice::isEnabled(PeerId peerId) const {
	const auto i = _enabled.find(peerId);
	return (i != end(_enabled)) && i->second;
}

void OneTimeVoice::setEnabled(PeerId peerId, bool enabled) {
	if (isEnabled(peerId) == enabled) {
		return;
	}
	if (enabled) {
		_enabled[peerId] = true;
	} else {
		_enabled.erase(peerId);
	}
	save();
	_changes.fire({});
}

bool OneTimeVoice::toggle(PeerId peerId) {
	const auto next = !isEnabled(peerId);
	setEnabled(peerId, next);
	return next;
}

rpl::producer<> OneTimeVoice::changes() const {
	return _changes.events();
}

void OneTimeVoice::load() {
	auto file = QFile(FilePath());
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto bytes = file.readAll();
	file.close();

	auto error = QJsonParseError();
	const auto doc = QJsonDocument::fromJson(bytes, &error);
	if (error.error != QJsonParseError::NoError || !doc.isArray()) {
		return;
	}
	for (const auto &value : doc.array()) {
		const auto peer = PeerId(value.toVariant().toULongLong());
		if (peer) {
			_enabled[peer] = true;
		}
	}
}

void OneTimeVoice::save() {
	auto array = QJsonArray();
	for (const auto &[peer, on] : _enabled) {
		if (on) {
			array.push_back(QString::number(peer.value));
		}
	}

	const auto path = FilePath();
	QDir().mkpath(QFileInfo(path).absolutePath());

	auto file = QFile(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return;
	}
	file.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
	file.close();
}

} // namespace Fenix
