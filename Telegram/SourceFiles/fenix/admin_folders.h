/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Admin folders: four chat folders gathering the groups and channels the user
owns or administers, the same ones Novagram Android makes
(org.fenixuz.folders.AdminFolders), so that both work on one account.

Telegram folders live on the server: they show on every device of the
account. Android and desktop recognize each other's folders by their titles,
which carry a "★" and one of the shared names, so turning the feature on
here adopts the folders made there instead of making a second set; and both
apply only what changed in the user's rights, keeping chats the user added
or removed by hand. Nothing is written to the account before the feature is
turned on in these settings.
*/
#pragma once

#include "base/timer.h"
#include "base/weak_ptr.h"
#include "data/data_peer_id.h"

class PeerData;

namespace Main {
class Session;
} // namespace Main

namespace Data {
class ChatFilter;
} // namespace Data

namespace Fenix {

class AdminFolders final : public base::has_weak_ptr {
public:
	// Android's order (and color index): groups owned, groups administered,
	// channels owned, channels administered.
	enum class Kind {
		GroupOwner,
		GroupAdmin,
		ChannelOwner,
		ChannelAdmin,
	};
	static constexpr auto kKindCount = 4;

	struct Result {
		// Set when nothing could be done; ready to show.
		QString blocked;
		// Folders cut to Telegram's chats-per-folder limit: title, kept.
		std::vector<std::pair<QString, int>> truncated;
		// Some folder could not be saved on the server.
		bool failed = false;
	};

	explicit AdminFolders(not_null<Main::Session*> session);
	~AdminFolders();

	// Whether this device keeps the folders: they were made or adopted
	// here and at least one of them is still on the account.
	[[nodiscard]] bool enabled() const;
	[[nodiscard]] rpl::producer<bool> enabledValue() const;

	// One operation runs at a time; the others are refused meanwhile.
	[[nodiscard]] bool busy() const;
	[[nodiscard]] rpl::producer<bool> busyValue() const;

	// Makes the folders, adopting ours already on the account.
	void enable(Fn<void(Result)> done);

	// Rebuilds them from the rights, dropping the user's own edits.
	void refresh(Fn<void(Result)> done);

	// Deletes ours from the account (all devices).
	void disable(Fn<void()> done);

private:
	struct Classified;
	struct Running;
	using Buckets = std::array<std::vector<PeerId>, kKindCount>;
	using Step = Fn<void(Fn<void()>)>;

	void load();
	void save();
	void clearState();
	void refreshEnabled();

	[[nodiscard]] bool ready() const;
	[[nodiscard]] bool claimed() const;
	void claim();
	void release();

	void scheduleSync();
	void sync();

	[[nodiscard]] Buckets classify() const;
	[[nodiscard]] Classified classify(not_null<PeerData*> peer) const;
	[[nodiscard]] const Data::ChatFilter *filterById(FilterId id) const;
	[[nodiscard]] const Data::ChatFilter *tracked(Kind kind) const;
	[[nodiscard]] const Data::ChatFilter *adoptable(
		Kind kind,
		const base::flat_set<FilterId> &taken) const;
	[[nodiscard]] FilterId freeId(const base::flat_set<FilterId> &taken) const;
	[[nodiscard]] int foldersCount() const;
	[[nodiscard]] int foldersLimit() const;
	[[nodiscard]] int chatsLimit() const;
	[[nodiscard]] QString limitText() const;

	// [rebuild] gives the folder our title, color and no rules, as making
	// or refreshing it does; otherwise only its chats change.
	[[nodiscard]] Data::ChatFilter makeFilter(
		Kind kind,
		FilterId id,
		const Data::ChatFilter *existing,
		const std::vector<PeerId> &members,
		bool rebuild) const;

	void saveFilter(Data::ChatFilter filter, Fn<void(bool)> done);
	void deleteFilter(FilterId id, Fn<void(bool)> done);
	void runChain(std::vector<Step> steps, Fn<void()> finished);
	void runStep(std::shared_ptr<Running> running, int index);

	const not_null<Main::Session*> _session;
	const QString _path;

	// The folder of each kind on the account, 0 for none.
	std::array<FilterId, kKindCount> _ids = {};
	// What each folder got from us at the last sync: a delta against it
	// tells a change in rights from the user's own edits.
	std::array<base::flat_set<PeerId>, kKindCount> _snapshot;
	// Kinds whose folder the user deleted: not made again on their own,
	// only by Refresh or by turning the feature on again.
	std::array<bool, kKindCount> _dismissed = {};

	rpl::variable<bool> _enabled = false;
	rpl::variable<bool> _busy = false;
	crl::time _busySince = 0;
	int _generation = 0;

	base::Timer _syncTimer;
	crl::time _lastSync = 0;

	rpl::lifetime _lifetime;

};

// Starts keeping the folders of every account that becomes active, for as
// long as its session lives. Once, at start.
void SetupAdminFolders();

[[nodiscard]] AdminFolders &GetAdminFolders(not_null<Main::Session*> session);

} // namespace Fenix
