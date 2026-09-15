/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/auto_text.h"

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
	return cWorkingDir() + u"tdata/fenix_autotext.json"_q;
}

} // namespace

AutoText &AutoText::Instance() {
	static AutoText instance;
	return instance;
}

AutoText::AutoText() {
	load();
}

TextWithTags AutoText::get(PeerId peerId) const {
	const auto i = _entries.find(peerId);
	return (i != end(_entries)) ? i->second : TextWithTags();
}

bool AutoText::isActive(PeerId peerId) const {
	return !get(peerId).text.isEmpty();
}

void AutoText::set(PeerId peerId, TextWithTags value) {
	if (value.text.trimmed().isEmpty()) {
		if (_entries.erase(peerId) > 0) {
			save();
			_changes.fire({});
		}
		return;
	}
	_entries[peerId] = std::move(value);
	save();
	_changes.fire({});
}

rpl::producer<> AutoText::changes() const {
	return _changes.events();
}

void AutoText::load() {
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
		auto content = TextWithTags{ obj.value(u"text"_q).toString() };
		for (const auto &tag : obj.value(u"tags"_q).toArray()) {
			const auto t = tag.toObject();
			content.tags.push_back({
				t.value(u"o"_q).toInt(),
				t.value(u"l"_q).toInt(),
				t.value(u"id"_q).toString() });
		}
		if (peer && !content.text.isEmpty()) {
			_entries[peer] = std::move(content);
		}
	}
}

void AutoText::save() {
	auto array = QJsonArray();
	for (const auto &[peer, content] : _entries) {
		auto tags = QJsonArray();
		for (const auto &tag : content.tags) {
			auto t = QJsonObject();
			t.insert(u"o"_q, tag.offset);
			t.insert(u"l"_q, tag.length);
			t.insert(u"id"_q, tag.id);
			tags.push_back(t);
		}
		auto obj = QJsonObject();
		obj.insert(u"peer"_q, QString::number(peer.value));
		obj.insert(u"text"_q, content.text);
		obj.insert(u"tags"_q, tags);
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
