/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

The saved history of an account: the marks of messages deleted by others
(DeletedMsg) and the previous versions of edited messages (EditHistory).
Each account keeps its own files next to its cache database, encrypted with
the local key and tagged with the account, so that message ids of private
chats and basic groups, which are counted per account, never meet another
account's records.
*/
#pragma once

class History;

namespace Main {
class Session;
} // namespace Main

namespace MTP {
class AuthKey;
using AuthKeyPtr = std::shared_ptr<AuthKey>;
} // namespace MTP

namespace Fenix {

inline constexpr auto kDeletedFileName = "novagram_deleted";
inline constexpr auto kEditsFileName = "novagram_edits";

// The full copies of kept messages, one file per chat, see kept_messages.h.
inline constexpr auto kKeptFilePrefix = "novagram_kept_";

// One encrypted file of an account's saved history.
class SessionFile final {
public:
	SessionFile(not_null<Main::Session*> session, const QString &name);

	// Empty when there is no file, it can't be read, or it belongs to
	// another account that used the same folder before.
	[[nodiscard]] QByteArray read() const;
	void write(const QByteArray &payload) const;
	void remove() const;

private:
	QString _basePath;
	QString _name;
	MTP::AuthKeyPtr _key;
	uint64 _owner = 0;

};

// Removes all the account's files whose names start with the prefix.
void RemoveSessionFiles(
	not_null<Main::Session*> session,
	const QString &prefix);

// The user deletes these messages of the history themselves: a server
// delete of them is not kept, and their saved copies and edit history are
// forgotten.
void RememberOwnDeletion(
	not_null<History*> history,
	const QVector<MTPint> &ids);

// The user clears or deletes the chat themselves: all its saved history is
// forgotten, so nothing of it comes back. Not for unloading the history.
void ForgetSavedChat(not_null<History*> history);

// The user deletes the chat's messages of these days themselves.
void ForgetSavedBetween(
	not_null<History*> history,
	TimeId minDate,
	TimeId maxDate);

// Deletes a kept message for good: its copy, its mark and its item.
void ForgetKeptMessage(not_null<History*> history, MsgId msgId);

// "Clear all" of the chat's Deleted messages: all its kept messages go,
// from the chat too.
void ForgetKeptChatMessages(not_null<History*> history);

// "Clear saved history": forgets the account's kept deleted messages,
// taking them out of the chats, and its saved edit history.
void ClearSavedHistory(not_null<Main::Session*> session);

// The account logs out: its saved history is removed from the disk.
void WipeSavedHistory(not_null<Main::Session*> session);

} // namespace Fenix
