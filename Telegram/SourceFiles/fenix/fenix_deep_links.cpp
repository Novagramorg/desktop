/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/fenix_deep_links.h"

#include "base/event_filter.h"
#include "base/unique_qptr.h"
#include "core/deep_links/deep_links_router.h"
#include "core/deep_links/deep_links_settings.h"
#include "core/deep_links/deep_links_types.h"
#include "fenix/sections/settings_chat_finder.h"
#include "fenix/sections/settings_fenix.h"
#include "fenix/sections/settings_novagram_bots.h"
#include "lang/lang_keys.h"
#include "ui/text/text_utilities.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_widgets.h"

#include <QtGui/QCursor>

namespace Fenix {

void RegisterDeepLinks(Core::DeepLinks::Router &router) {
	using namespace Core::DeepLinks;

	router.add(u"settings"_q, {
		.path = u"novagram"_q,
		.action = SettingsSection{ ::Settings::FenixId() },
	});
	router.add(u"settings"_q, {
		.path = u"novagram/bots"_q,
		.action = SettingsSection{ ::Settings::NovagramBotsId() },
	});
	router.add(u"settings"_q, {
		.path = u"novagram/chat-finder"_q,
		.action = SettingsSection{ ::Settings::FenixChatFinderId() },
	});
}

void SetupCopyDeepLink(
		not_null<Ui::SettingsButton*> button,
		not_null<Window::SessionController*> controller,
		::Settings::Type section,
		const QString &controlId) {
	const auto menu = button->lifetime().make_state<
		base::unique_qptr<Ui::PopupMenu>>();
	base::install_event_filter(button, [=](not_null<QEvent*> e) {
		if (e->type() != QEvent::ContextMenu) {
			return base::EventFilterResult::Continue;
		}
		const auto link = Core::DeepLinks::SettingsDeepLink(
			section,
			controlId);
		if (link.isEmpty()) {
			return base::EventFilterResult::Continue;
		}
		*menu = base::make_unique_q<Ui::PopupMenu>(
			button,
			st::popupMenuWithIcons);
		(*menu)->addAction(tr::lng_context_copy_link(tr::now), [=] {
			TextUtilities::SetClipboardText(TextForMimeData::Simple(link));
			controller->showToast(
				tr::lng_channel_public_link_copied(tr::now));
		}, &st::menuIconLink);
		(*menu)->popup(QCursor::pos());
		return base::EventFilterResult::Cancel;
	});
}

} // namespace Fenix
