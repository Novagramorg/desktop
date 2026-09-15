/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/text_style_box.h"

#include "fenix/fenix_settings.h"
#include "lang/lang_keys.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/checkbox.h"

#include <memory>

namespace Fenix {

void FenixTextStyleBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(tr::lng_fenix_text_style());

	const auto group = std::make_shared<Ui::RadiobuttonGroup>(
		int(Get().textStyle()));
	const auto add = [&](TextStyle style, tr::phrase<> label) {
		box->addRow(object_ptr<Ui::Radiobutton>(
			box,
			group,
			int(style),
			label(tr::now)));
	};
	add(TextStyle::None, tr::lng_fenix_text_style_off);
	add(TextStyle::Regular, tr::lng_fenix_text_style_regular);
	add(TextStyle::Bold, tr::lng_fenix_text_style_bold);
	add(TextStyle::Italic, tr::lng_fenix_text_style_italic);
	add(TextStyle::Mono, tr::lng_fenix_text_style_mono);
	add(TextStyle::Strike, tr::lng_fenix_text_style_strike);
	add(TextStyle::Underline, tr::lng_fenix_text_style_underline);
	add(TextStyle::Spoiler, tr::lng_fenix_text_style_spoiler);
	add(TextStyle::Quote, tr::lng_fenix_text_style_quote);

	group->setChangedCallback([=](int value) {
		Get().setTextStyle(static_cast<TextStyle>(value));
	});

	box->addButton(tr::lng_box_done(), [=] { box->closeBox(); });
}

} // namespace Fenix
