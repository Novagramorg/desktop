/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/deleted_msg.h"

#include "base/unixtime.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"

#include <QtCore/QDataStream>

namespace Fenix {
namespace {

constexpr auto kSaveDelay = 3 * crl::time(1000);

// The oldest marks go first when there are more.
constexpr auto kMaxRecords = 20000;

// An own delete the server didn't report in this time is not waited for.
constexpr auto kOwnDeletionTimeout = 120 * crl::time(1000);

// The server deletes a message when its auto-delete timer runs out, at about
// the time the app's own timer does.
constexpr auto kTtlSlack = TimeId(30);

} // namespace

DeletedMsg::DeletedMsg(not_null<Main::Session*> session)
: _file(session, QString::fromLatin1(kDeletedFileName))
, _saveTimer([=] { save(); }) {
	load();
}

DeletedMsg::~DeletedMsg() {
	if (_saveTimer.isActive()) {
		save();
	}
}

void DeletedMsg::recordDeletion(PeerId peerId, MsgId msgId, DeletedBy by) {
	if (_wiped) {
		return;
	}
	const auto key = FullMsgId(peerId, msgId);
	const auto i = _records.find(key);
	if (i != end(_records) && i->second.by == by) {
		return;
	}
	_records[key] = Record{ .by = by, .at = base::unixtime::now() };
	trim();
	saveDelayed();
}

std::optional<DeletedBy> DeletedMsg::whoDeleted(
		PeerId peerId,
		MsgId msgId) const {
	const auto i = _records.find(FullMsgId(peerId, msgId));
	if (i == end(_records)) {
		return std::nullopt;
	}
	return i->second.by;
}

std::vector<FullMsgId> DeletedMsg::all() const {
	auto result = std::vector<FullMsgId>();
	result.reserve(_records.size());
	for (const auto &[key, record] : _records) {
		result.push_back(key);
	}
	return result;
}

void DeletedMsg::forget(PeerId peerId, MsgId msgId) {
	if (_records.erase(FullMsgId(peerId, msgId)) > 0) {
		saveDelayed();
	}
}

void DeletedMsg::forgetChat(PeerId peerId) {
	auto removed = false;
	for (auto i = begin(_records); i != end(_records);) {
		if (i->first.peer == peerId) {
			i = _records.erase(i);
			removed = true;
		} else {
			++i;
		}
	}
	if (removed) {
		saveDelayed();
	}
}

void DeletedMsg::clear() {
	if (_wiped) {
		return;
	}
	_records.clear();
	_saveTimer.cancel();
	save();
}

void DeletedMsg::markOwnDeletion(
		PeerId peerId,
		const QVector<MTPint> &ids) {
	const auto now = crl::now();
	for (auto i = begin(_own); i != end(_own);) {
		if (i->second + kOwnDeletionTimeout <= now) {
			i = _own.erase(i);
		} else {
			++i;
		}
	}
	for (const auto &id : ids) {
		const auto key = FullMsgId(peerId, MsgId(id.v));
		_own[key] = now;
		forget(key.peer, key.msg);
	}
}

bool DeletedMsg::takeOwnDeletion(PeerId peerId, MsgId msgId) {
	const auto i = _own.find(FullMsgId(peerId, msgId));
	if (i == end(_own)) {
		return false;
	}
	const auto fresh = (i->second + kOwnDeletionTimeout > crl::now());
	_own.erase(i);
	return fresh;
}

void DeletedMsg::importLegacy(
		const std::vector<std::pair<FullMsgId, DeletedBy>> &list) {
	if (_wiped || list.empty()) {
		return;
	}
	const auto now = base::unixtime::now();
	for (const auto &[key, by] : list) {
		_records.emplace(key, Record{ .by = by, .at = now });
	}
	trim();
	_saveTimer.cancel();
	save();
}

void DeletedMsg::wipe() {
	_wiped = true;
	_saveTimer.cancel();
	_records.clear();
	_own.clear();
}

void DeletedMsg::trim() {
	if (int(_records.size()) <= kMaxRecords) {
		return;
	}
	// Drop down to 90% at once, so trimming doesn't run on every new mark.
	// The marks older than the threshold go first, then the ones of the
	// threshold's own second, until enough are dropped.
	auto dates = std::vector<TimeId>();
	dates.reserve(_records.size());
	for (const auto &[key, record] : _records) {
		dates.push_back(record.at);
	}
	auto left = int(_records.size()) - (kMaxRecords * 9 / 10);
	std::nth_element(begin(dates), begin(dates) + left, end(dates));
	const auto threshold = dates[left];
	for (auto i = begin(_records); i != end(_records);) {
		if (i->second.at < threshold) {
			i = _records.erase(i);
			--left;
		} else {
			++i;
		}
	}
	for (auto i = begin(_records); i != end(_records) && left > 0;) {
		if (i->second.at == threshold) {
			i = _records.erase(i);
			--left;
		} else {
			++i;
		}
	}
}

void DeletedMsg::saveDelayed() {
	if (!_wiped && !_saveTimer.isActive()) {
		_saveTimer.callOnce(kSaveDelay);
	}
}

void DeletedMsg::load() {
	const auto payload = _file.read();
	if (payload.isEmpty()) {
		return;
	}
	QDataStream stream(payload);
	stream.setVersion(QDataStream::Qt_5_1);
	auto count = quint32();
	stream >> count;
	if (stream.status() != QDataStream::Ok
		|| count > quint32(4 * kMaxRecords)) {
		return;
	}
	auto records = std::map<FullMsgId, Record>();
	for (auto i = quint32(); i != count; ++i) {
		auto peer = quint64();
		auto msg = qint64();
		auto by = qint32();
		auto at = qint32();
		stream >> peer >> msg >> by >> at;
		if (stream.status() != QDataStream::Ok) {
			return;
		}
		const auto peerId = DeserializePeerId(peer);
		if (!peerId
			|| !IsServerMsgId(MsgId(msg))
			|| by < int(DeletedBy::Other)
			|| by > int(DeletedBy::Channel)) {
			continue;
		}
		records[FullMsgId(peerId, MsgId(msg))] = Record{
			.by = DeletedBy(by),
			.at = TimeId(at),
		};
	}
	_records = std::move(records);
}

void DeletedMsg::save() {
	if (_wiped) {
		return;
	}
	auto payload = QByteArray();
	{
		QDataStream stream(&payload, QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_5_1);
		stream << quint32(_records.size());
		for (const auto &[key, record] : _records) {
			stream
				<< quint64(SerializePeerId(key.peer))
				<< qint64(key.msg.bare)
				<< qint32(record.by)
				<< qint32(record.at);
		}
	}
	_file.write(payload);
}

bool KeepDeleted(not_null<HistoryItem*> item) {
	// The user's own messages are not kept: the server doesn't say who
	// deleted one, and it is nearly always the user, on another device.
	if (!IsServerMsgId(item->id)
		|| item->out()
		|| item->isService()
		|| item->isEphemeral()) {
		return false;
	}
	const auto history = item->history();
	auto &deleted = GetDeleted(&history->session());
	if (deleted.takeOwnDeletion(history->peer->id, item->id)) {
		return false;
	}
	const auto destroyAt = item->ttlDestroyAt();
	if (destroyAt && destroyAt <= base::unixtime::now() + kTtlSlack) {
		return false;
	}
	return true;
}

} // namespace Fenix
