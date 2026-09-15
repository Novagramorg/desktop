/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/chat_lock.h"

#include "settings.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Fenix {
namespace {

[[nodiscard]] QString FilePath() {
	return cWorkingDir() + u"tdata/fenix_chatlock.json"_q;
}

[[nodiscard]] QString Hash(const QString &passcode) {
	if (passcode.isEmpty()) {
		return QString();
	}
	return QString::fromLatin1(QCryptographicHash::hash(
		passcode.toUtf8(),
		QCryptographicHash::Sha256).toHex());
}

} // namespace

ChatLock &ChatLock::Instance() {
	static ChatLock instance;
	return instance;
}

ChatLock::ChatLock() {
	load();
}

bool ChatLock::isLocked(PeerId peerId) const {
	return _commonIds.contains(peerId) || _individual.contains(peerId);
}

bool ChatLock::isIndividual(PeerId peerId) const {
	return _individual.contains(peerId);
}

bool ChatLock::hasCommonPassword() const {
	return !_commonHash.isEmpty();
}

bool ChatLock::hasAnyLock() const {
	return !_commonIds.empty() || !_individual.empty();
}

QString ChatLock::effectiveHash(PeerId peerId) const {
	const auto i = _individual.find(peerId);
	return (i != _individual.end()) ? i->second : _commonHash;
}

bool ChatLock::check(PeerId peerId, const QString &input) const {
	const auto hash = effectiveHash(peerId);
	return !hash.isEmpty() && (Hash(input) == hash);
}

void ChatLock::changeCommonPassword(const QString &passcode) {
	_commonHash = Hash(passcode);
	save();
}

void ChatLock::addCommonLock(PeerId peerId) {
	if (_commonIds.emplace(peerId).second) {
		save();
	}
}

void ChatLock::addIndividualLock(PeerId peerId, const QString &passcode) {
	// An individual lock owns the chat exclusively — drop any common membership.
	_commonIds.remove(peerId);
	_individual[peerId] = Hash(passcode);
	save();
}

void ChatLock::unlock(PeerId peerId) {
	auto changed = _commonIds.remove(peerId);
	if (_individual.remove(peerId)) {
		changed = true;
	}
	if (changed) {
		save();
	}
}

void ChatLock::removeEverything() {
	const auto changed = !_commonIds.empty()
		|| !_individual.empty()
		|| !_commonHash.isEmpty();
	_commonIds.clear();
	_individual.clear();
	_commonHash = QString();
	if (changed) {
		save();
	}
}

bool ChatLock::isUnlocked(PeerId peerId) const {
	return _unlocked.contains(peerId);
}

void ChatLock::markUnlocked(PeerId peerId) {
	_unlocked.emplace(peerId);
}

void ChatLock::relockAllExcept(PeerId keep) {
	if (_unlocked.empty()) {
		return;
	}
	const auto keepUnlocked = keep && _unlocked.contains(keep);
	_unlocked.clear();
	if (keepUnlocked) {
		_unlocked.emplace(keep);
	}
}

void ChatLock::load() {
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
	_commonHash = root.value(u"common"_q).toString();
	for (const auto &value : root.value(u"common_ids"_q).toArray()) {
		const auto peer = PeerId(value.toVariant().toULongLong());
		if (peer) {
			_commonIds.emplace(peer);
		}
	}
	const auto individual = root.value(u"individual"_q).toObject();
	for (auto i = individual.begin(); i != individual.end(); ++i) {
		const auto peer = PeerId(i.key().toULongLong());
		const auto hash = i.value().toString();
		if (peer && !hash.isEmpty()) {
			_individual[peer] = hash;
		}
	}
}

void ChatLock::save() {
	auto commonIds = QJsonArray();
	for (const auto &peer : _commonIds) {
		commonIds.push_back(QString::number(peer.value));
	}
	auto individual = QJsonObject();
	for (const auto &[peer, hash] : _individual) {
		individual.insert(QString::number(peer.value), hash);
	}

	auto root = QJsonObject();
	root.insert(u"common"_q, _commonHash);
	root.insert(u"common_ids"_q, commonIds);
	root.insert(u"individual"_q, individual);

	const auto path = FilePath();
	QDir().mkpath(QFileInfo(path).absolutePath());

	auto file = QFile(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return;
	}
	file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	file.close();
}

} // namespace Fenix
