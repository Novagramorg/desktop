/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/edit_history.h"

#include "settings.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Fenix {
namespace {

constexpr auto kMaxVersionsPerMessage = 30;

[[nodiscard]] QString FilePath() {
	return cWorkingDir() + u"tdata/fenix_edits.json"_q;
}

} // namespace

EditHistory &EditHistory::Instance() {
	static EditHistory instance;
	return instance;
}

EditHistory::EditHistory() {
	load();
}

void EditHistory::recordEdit(
		PeerId peerId,
		MsgId msgId,
		QString text,
		TimeId date) {
	auto &list = _records[Key{ peerId, msgId }];
	if (!list.empty() && list.back().text == text) {
		return;
	}
	list.push_back({ std::move(text), date });
	if (list.size() > kMaxVersionsPerMessage) {
		list.erase(begin(list), end(list) - kMaxVersionsPerMessage);
	}
	scheduleSave();
	_changes.fire({});
}

std::vector<EditHistory::Version> EditHistory::versions(
		PeerId peerId,
		MsgId msgId) const {
	const auto i = _records.find(Key{ peerId, msgId });
	return (i != end(_records)) ? i->second : std::vector<Version>();
}

bool EditHistory::has(PeerId peerId, MsgId msgId) const {
	const auto i = _records.find(Key{ peerId, msgId });
	return (i != end(_records)) && !i->second.empty();
}

void EditHistory::forget(PeerId peerId, MsgId msgId) {
	if (_records.erase(Key{ peerId, msgId }) > 0) {
		scheduleSave();
		_changes.fire({});
	}
}

void EditHistory::forgetAll(PeerId peerId) {
	bool changed = false;
	for (auto it = _records.begin(); it != _records.end();) {
		if (it->first.peer == peerId) {
			it = _records.erase(it);
			changed = true;
		} else {
			++it;
		}
	}
	if (changed) {
		scheduleSave();
		_changes.fire({});
	}
}

rpl::producer<> EditHistory::changes() const {
	return _changes.events();
}

void EditHistory::scheduleSave() {
	// Simple synchronous save for now; small JSON, infrequent writes.
	save();
}

void EditHistory::load() {
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
	const auto items = doc.array();
	for (const auto &value : items) {
		if (!value.isObject()) {
			continue;
		}
		const auto obj = value.toObject();
		const auto peer = PeerId(
			obj.value(u"peer"_q).toVariant().toULongLong());
		const auto msg = MsgId(
			obj.value(u"msg"_q).toVariant().toLongLong());
		if (!peer || !msg.bare) {
			continue;
		}
		auto list = std::vector<Version>();
		const auto versions = obj.value(u"versions"_q).toArray();
		for (const auto &v : versions) {
			if (!v.isObject()) {
				continue;
			}
			const auto vo = v.toObject();
			list.push_back({
				vo.value(u"text"_q).toString(),
				TimeId(vo.value(u"date"_q).toVariant().toLongLong()),
			});
		}
		if (!list.empty()) {
			_records[Key{ peer, msg }] = std::move(list);
		}
	}
}

void EditHistory::save() {
	auto array = QJsonArray();
	for (const auto &[key, list] : _records) {
		auto versions = QJsonArray();
		for (const auto &version : list) {
			auto vo = QJsonObject();
			vo.insert(u"text"_q, version.text);
			vo.insert(u"date"_q, qint64(version.date));
			versions.push_back(vo);
		}
		auto obj = QJsonObject();
		obj.insert(u"peer"_q, QString::number(key.peer.value));
		obj.insert(u"msg"_q, qint64(key.msg.bare));
		obj.insert(u"versions"_q, versions);
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
