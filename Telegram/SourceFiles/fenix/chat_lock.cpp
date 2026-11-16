/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/chat_lock.h"

#include "settings.h"
#include "base/bytes.h"
#include "base/openssl_help.h"
#include "base/random.h"

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

// Stored format: "pbkdf2-sha512$<iterations>$<salt hex>$<hash hex>".
// The same KDF and iteration count Telegram uses for the cloud password, so
// a short PIN can't be brute-forced from fenix_chatlock.json in a moment.
constexpr auto kKdfPrefix = "pbkdf2-sha512";
constexpr auto kKdfIterations = 100000;
constexpr auto kSaltSize = 16;
constexpr auto kLegacySha256HexSize = 64;

[[nodiscard]] QByteArray ToByteArray(const bytes::vector &value) {
	return QByteArray(
		reinterpret_cast<const char*>(value.data()),
		int(value.size()));
}

[[nodiscard]] QByteArray Derive(
		const QString &passcode,
		const QByteArray &salt,
		int iterations) {
	return ToByteArray(openssl::Pbkdf2Sha512(
		bytes::make_span(passcode.toUtf8()),
		bytes::make_span(salt),
		iterations));
}

// Compares without an early exit, so the time taken doesn't leak how many
// leading bytes matched.
[[nodiscard]] bool ConstantTimeEqual(const QByteArray &a, const QByteArray &b) {
	if (a.size() != b.size()) {
		return false;
	}
	auto diff = 0;
	for (auto i = 0; i != a.size(); ++i) {
		diff |= (uchar(a[i]) ^ uchar(b[i]));
	}
	return (diff == 0);
}

[[nodiscard]] QString Hash(const QString &passcode) {
	if (passcode.isEmpty()) {
		return QString();
	}
	auto salt = bytes::vector(kSaltSize);
	base::RandomFill(bytes::make_span(salt));
	const auto saltBytes = ToByteArray(salt);
	return u"%1$%2$%3$%4"_q.arg(
		QString::fromLatin1(kKdfPrefix),
		QString::number(kKdfIterations),
		QString::fromLatin1(saltBytes.toHex()),
		QString::fromLatin1(Derive(
			passcode,
			saltBytes,
			kKdfIterations).toHex()));
}

[[nodiscard]] bool IsLegacyHash(const QString &stored) {
	return (stored.size() == kLegacySha256HexSize)
		&& !stored.contains(QChar('$'));
}

// Verifies [input] against a stored hash in either the current PBKDF2 format
// or the old unsalted SHA-256 hex one (from before the format change).
[[nodiscard]] bool Verify(const QString &stored, const QString &input) {
	if (stored.isEmpty() || input.isEmpty()) {
		return false;
	} else if (IsLegacyHash(stored)) {
		return ConstantTimeEqual(
			QCryptographicHash::hash(
				input.toUtf8(),
				QCryptographicHash::Sha256).toHex(),
			stored.toLatin1().toLower());
	}
	const auto parts = stored.split(QChar('$'));
	if (parts.size() != 4 || parts[0] != QString::fromLatin1(kKdfPrefix)) {
		return false;
	}
	auto ok = false;
	const auto iterations = parts[1].toInt(&ok);
	if (!ok || iterations <= 0) {
		return false;
	}
	const auto salt = QByteArray::fromHex(parts[2].toLatin1());
	const auto expected = QByteArray::fromHex(parts[3].toLatin1());
	if (salt.isEmpty() || expected.isEmpty()) {
		return false;
	}
	return ConstantTimeEqual(Derive(input, salt, iterations), expected);
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

bool ChatLock::check(PeerId peerId, const QString &input) {
	const auto i = _individual.find(peerId);
	if (i == _individual.end()) {
		return checkCommon(input);
	} else if (!Verify(i->second, input)) {
		return false;
	} else if (IsLegacyHash(i->second)) {
		// Re-store the passcode in the salted format now that we know it.
		i->second = Hash(input);
		save();
	}
	return true;
}

bool ChatLock::checkCommon(const QString &input) {
	if (!Verify(_commonHash, input)) {
		return false;
	} else if (IsLegacyHash(_commonHash)) {
		// Re-store the passcode in the salted format now that we know it.
		_commonHash = Hash(input);
		save();
	}
	return true;
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
