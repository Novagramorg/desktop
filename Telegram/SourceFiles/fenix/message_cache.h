/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

The account's cache of others' messages it saw in the last month: their
server data, in an encrypted database next to Telegram's own cache. A
message deleted or edited while it isn't loaded, even while Novagram was
closed, still has its copy there: it is kept as deleted (see
fenix/kept_messages.h), or its earlier text goes to its edit history (see
fenix/edit_history.h). Works while Save Deleted Messages or Save Edit
History is on; with both off it is emptied.
*/
#pragma once

#include "storage/storage_databases.h"

#include <optional>

namespace Main {
class Session;
} // namespace Main

namespace Fenix {

class MessageCache final {
public:
	explicit MessageCache(not_null<Main::Session*> session);
	~MessageCache();

	// A message seen: stored unless it is there already.
	void remember(const MTPMessage &message);

	// A message edited: its copy is replaced. When it isn't loaded, its
	// earlier text is taken from the copy it replaces.
	void edited(const MTPMessage &message, bool loaded);

	// A message deleted while it isn't loaded: kept from its copy. The
	// channel is empty for private chats and groups, whose message ids are
	// the account's own: their deletes come without the chat.
	void deleted(PeerId channelId, MsgId msgId);

	// The user deletes it themselves.
	void forget(PeerId peerId, MsgId msgId);

	void clear();

	// The account logs out: nothing is stored any more.
	void wipe();

	[[nodiscard]] rpl::producer<int64> sizeValue() const;

private:
	[[nodiscard]] Storage::Cache::Database &database();
	[[nodiscard]] Storage::Cache::Database *activeDatabase();

	const not_null<Main::Session*> _session;
	std::optional<Storage::DatabasePointer> _database;
	bool _wiped = false;
	rpl::lifetime _lifetime;

};

[[nodiscard]] MessageCache &GetMessageCache(
	not_null<Main::Session*> session);

// The account logs out before its cache was opened: its files go.
void RemoveMessageCacheFiles(not_null<Main::Session*> session);

} // namespace Fenix
