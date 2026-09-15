/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/auto_text_box.h"

#include "fenix/auto_text.h"
#include "lang/lang_keys.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/input_field.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"

namespace Fenix {

void FenixAutoTextBox(not_null<Ui::GenericBox*> box, PeerId peerId) {
	box->setTitle(tr::lng_fenix_auto_text());

	const auto field = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::shareComment,
		Ui::InputField::Mode::MultiLine,
		tr::lng_fenix_auto_text_placeholder()));
	field->setTextWithTags(GetAutoText().get(peerId));
	field->setMaxLength(1024);
	box->setFocusCallback([=] { field->setFocusFast(); });

	box->addButton(tr::lng_settings_save(), [=] {
		GetAutoText().set(peerId, field->getTextWithAppliedMarkdown());
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

} // namespace Fenix
