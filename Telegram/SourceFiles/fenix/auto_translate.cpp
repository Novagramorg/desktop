/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/auto_translate.h"

#include "settings.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Fenix {
namespace {

[[nodiscard]] QString FilePath() {
	return cWorkingDir() + u"tdata/fenix_autotranslate.json"_q;
}

} // namespace

AutoTranslate &AutoTranslate::Instance() {
	static AutoTranslate instance;
	return instance;
}

AutoTranslate::AutoTranslate() {
	load();
}

QString AutoTranslate::getLang(PeerId peerId) const {
	const auto i = _langs.find(peerId);
	return (i != end(_langs)) ? i->second : QString();
}

bool AutoTranslate::isEnabled(PeerId peerId) const {
	return !getLang(peerId).isEmpty();
}

void AutoTranslate::setLang(PeerId peerId, QString lang) {
	lang = lang.trimmed();
	if (lang.isEmpty() || lang == u"not"_q) {
		if (_langs.erase(peerId) > 0) {
			save();
			_changes.fire({});
		}
		return;
	}
	const auto i = _langs.find(peerId);
	if (i != end(_langs) && i->second == lang) {
		return;
	}
	_langs[peerId] = std::move(lang);
	save();
	_changes.fire({});
}

rpl::producer<> AutoTranslate::changes() const {
	return _changes.events();
}

void AutoTranslate::load() {
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
		if (!value.isObject()) {
			continue;
		}
		const auto obj = value.toObject();
		const auto peer = PeerId(
			obj.value(u"peer"_q).toVariant().toULongLong());
		const auto lang = obj.value(u"lang"_q).toString();
		if (peer && !lang.isEmpty()) {
			_langs[peer] = lang;
		}
	}
}

void AutoTranslate::save() {
	auto array = QJsonArray();
	for (const auto &[peer, lang] : _langs) {
		auto obj = QJsonObject();
		obj.insert(u"peer"_q, QString::number(peer.value));
		obj.insert(u"lang"_q, lang);
		array.push_back(obj);
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
