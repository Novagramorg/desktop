/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/edit_history.h"

#include "fenix/fenix_settings.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"

#include <QtCore/QDataStream>

namespace Fenix {
namespace {

constexpr auto kSaveDelay = 5 * crl::time(1000);
constexpr auto kMaxVersionsPerMessage = 30;

// The whole file is written again on a change, so its size is kept small:
// the messages edited longest ago go first when the texts take more.
constexpr auto kMaxTextSize = int64(2 * 1024 * 1024);

[[nodiscard]] int64 TextSize(const std::vector<EditHistory::Version> &list) {
	auto result = int64();
	for (const auto &version : list) {
		result += version.text.size();
	}
	return result;
}

[[nodiscard]] TimeId LastDate(const std::vector<EditHistory::Version> &list) {
	return list.empty() ? TimeId() : list.back().date;
}

} // namespace

EditHistory::EditHistory(not_null<Main::Session*> session)
: _file(session, QString::fromLatin1(kEditsFileName))
, _saveTimer([=] { save(); }) {
	load();
}

EditHistory::~EditHistory() {
	if (_saveTimer.isActive()) {
		save();
	}
}

void EditHistory::recordEdit(
		PeerId peerId,
		MsgId msgId,
		QString text,
		TimeId date) {
	if (_wiped || !IsServerMsgId(msgId)) {
		return;
	}
	auto &list = _records[FullMsgId(peerId, msgId)];
	if (!list.empty() && list.back().text == text) {
		return;
	}
	_textSize += text.size();
	list.push_back({ std::move(text), date });
	if (int(list.size()) > kMaxVersionsPerMessage) {
		const auto drop = int(list.size()) - kMaxVersionsPerMessage;
		for (auto i = 0; i != drop; ++i) {
			_textSize -= list[i].text.size();
		}
		list.erase(begin(list), begin(list) + drop);
	}
	trim();
	saveDelayed();
}

std::vector<EditHistory::Version> EditHistory::versions(
		PeerId peerId,
		MsgId msgId) const {
	const auto i = _records.find(FullMsgId(peerId, msgId));
	return (i != end(_records)) ? i->second : std::vector<Version>();
}

bool EditHistory::has(PeerId peerId, MsgId msgId) const {
	const auto i = _records.find(FullMsgId(peerId, msgId));
	return (i != end(_records)) && !i->second.empty();
}

void EditHistory::forget(PeerId peerId, MsgId msgId) {
	const auto i = _records.find(FullMsgId(peerId, msgId));
	if (i == end(_records)) {
		return;
	}
	_textSize -= TextSize(i->second);
	_records.erase(i);
	saveDelayed();
}

void EditHistory::forgetChat(PeerId peerId) {
	auto removed = false;
	for (auto i = begin(_records); i != end(_records);) {
		if (i->first.peer == peerId) {
			_textSize -= TextSize(i->second);
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

void EditHistory::clear() {
	if (_wiped) {
		return;
	}
	_records.clear();
	_textSize = 0;
	_saveTimer.cancel();
	save();
}

void EditHistory::importLegacy(
		std::vector<std::pair<FullMsgId, std::vector<Version>>> list) {
	if (_wiped || list.empty()) {
		return;
	}
	for (auto &[key, versions] : list) {
		if (versions.empty() || _records.contains(key)) {
			continue;
		}
		if (int(versions.size()) > kMaxVersionsPerMessage) {
			versions.erase(
				begin(versions),
				end(versions) - kMaxVersionsPerMessage);
		}
		_textSize += TextSize(versions);
		_records.emplace(key, std::move(versions));
	}
	trim();
	_saveTimer.cancel();
	save();
}

void EditHistory::wipe() {
	_wiped = true;
	_saveTimer.cancel();
	_records.clear();
	_textSize = 0;
}

void EditHistory::trim() {
	if (_textSize <= kMaxTextSize) {
		return;
	}
	// Drop down to 90% at once, so trimming doesn't run on every edit.
	auto order = std::vector<std::pair<TimeId, FullMsgId>>();
	order.reserve(_records.size());
	for (const auto &[key, list] : _records) {
		order.emplace_back(LastDate(list), key);
	}
	ranges::sort(order);
	const auto goal = kMaxTextSize * 9 / 10;
	for (const auto &[date, key] : order) {
		if (_textSize <= goal) {
			break;
		}
		const auto i = _records.find(key);
		_textSize -= TextSize(i->second);
		_records.erase(i);
	}
}

void EditHistory::saveDelayed() {
	if (!_wiped && !_saveTimer.isActive()) {
		_saveTimer.callOnce(kSaveDelay);
	}
}

void EditHistory::load() {
	const auto payload = _file.read();
	if (payload.isEmpty()) {
		return;
	}
	QDataStream stream(payload);
	stream.setVersion(QDataStream::Qt_5_1);
	auto count = quint32();
	stream >> count;
	if (stream.status() != QDataStream::Ok) {
		return;
	}
	auto records = std::map<FullMsgId, std::vector<Version>>();
	auto size = int64();
	for (auto i = quint32(); i != count; ++i) {
		auto peer = quint64();
		auto msg = qint64();
		auto versionsCount = quint32();
		stream >> peer >> msg >> versionsCount;
		if (stream.status() != QDataStream::Ok
			|| versionsCount > quint32(kMaxVersionsPerMessage)) {
			return;
		}
		auto list = std::vector<Version>();
		list.reserve(versionsCount);
		for (auto j = quint32(); j != versionsCount; ++j) {
			auto text = QString();
			auto date = qint32();
			stream >> text >> date;
			if (stream.status() != QDataStream::Ok) {
				return;
			}
			list.push_back({ std::move(text), TimeId(date) });
		}
		const auto peerId = DeserializePeerId(peer);
		if (peerId && IsServerMsgId(MsgId(msg)) && !list.empty()) {
			auto &now = records[FullMsgId(peerId, MsgId(msg))];
			size += TextSize(list) - TextSize(now);
			now = std::move(list);
		}
	}
	_records = std::move(records);
	_textSize = size;
}

void EditHistory::save() {
	if (_wiped) {
		return;
	}
	auto payload = QByteArray();
	{
		QDataStream stream(&payload, QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_5_1);
		stream << quint32(_records.size());
		for (const auto &[key, list] : _records) {
			stream
				<< quint64(SerializePeerId(key.peer))
				<< qint64(key.msg.bare)
				<< quint32(list.size());
			for (const auto &version : list) {
				stream << version.text << qint32(version.date);
			}
		}
	}
	_file.write(payload);
}

bool HasEditHistoryAction(not_null<HistoryItem*> item) {
	if (!IsServerMsgId(item->id) || item->out()) {
		return false;
	}
	const auto history = item->history();
	return GetEdits(&history->session()).has(history->peer->id, item->id)
		|| (Get().saveEditHistory() && item->Has<HistoryMessageEdited>());
}

} // namespace Fenix
