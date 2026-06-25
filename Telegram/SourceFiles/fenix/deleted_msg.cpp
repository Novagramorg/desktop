/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/deleted_msg.h"

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
	return cWorkingDir() + u"tdata/fenix_deleted.json"_q;
}

} // namespace

DeletedMsg &DeletedMsg::Instance() {
	static DeletedMsg instance;
	return instance;
}

DeletedMsg::DeletedMsg() {
	load();
}

void DeletedMsg::recordDeletion(PeerId peerId, MsgId msgId, DeletedBy by) {
	const auto key = Key{ peerId, msgId };
	const auto [it, inserted] = _records.try_emplace(key, by);
	if (!inserted && it->second == by) {
		return;
	}
	it->second = by;
	scheduleSave();
	_changes.fire({});
}

std::optional<DeletedBy> DeletedMsg::whoDeleted(
		PeerId peerId,
		MsgId msgId) const {
	const auto i = _records.find(Key{ peerId, msgId });
	if (i == end(_records)) {
		return std::nullopt;
	}
	return i->second;
}

void DeletedMsg::forget(PeerId peerId, MsgId msgId) {
	if (_records.erase(Key{ peerId, msgId }) > 0) {
		scheduleSave();
		_changes.fire({});
	}
}

void DeletedMsg::forgetAll(PeerId peerId) {
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

int DeletedMsg::countFor(PeerId peerId) const {
	int result = 0;
	for (const auto &[key, by] : _records) {
		if (key.peer == peerId) {
			++result;
		}
	}
	return result;
}

void DeletedMsg::markNextAsMine() {
	_markNextAsMine = true;
}

bool DeletedMsg::takeNextAsMine() {
	const auto was = _markNextAsMine;
	_markNextAsMine = false;
	return was;
}

rpl::producer<> DeletedMsg::changes() const {
	return _changes.events();
}

void DeletedMsg::scheduleSave() {
	// Simple synchronous save for now; small JSON, infrequent writes.
	save();
}

void DeletedMsg::load() {
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
		const auto peer = PeerId(obj.value(u"peer"_q).toVariant().toULongLong());
		const auto msg = MsgId(obj.value(u"msg"_q).toVariant().toLongLong());
		const auto by = obj.value(u"by"_q).toInt(0);
		if (!peer || !msg.bare) {
			continue;
		}
		_records[Key{ peer, msg }] = static_cast<DeletedBy>(by);
	}
}

void DeletedMsg::save() {
	auto array = QJsonArray();
	for (const auto &[key, by] : _records) {
		auto obj = QJsonObject();
		obj.insert(u"peer"_q, QString::number(key.peer.value));
		obj.insert(u"msg"_q, qint64(key.msg.bare));
		obj.insert(u"by"_q, int(by));
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
