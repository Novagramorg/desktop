/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/photo_to_text.h"

#include "lang/lang_keys.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>

namespace Fenix {

#ifndef Q_OS_WIN
// Platforms without a bundled on-device OCR engine. The Windows implementation
// lives in photo_to_text_win.cpp (compiled only on Windows).
bool OcrSupported() {
	return false;
}

void OcrRecognize(QImage image, Fn<void(std::optional<QString>)> done) {
	if (done) {
		done(std::nullopt);
	}
}
#endif // !Q_OS_WIN

void ShowPhotoToText(
		not_null<Window::SessionController*> controller,
		QImage image) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::lng_fenix_ocr_title());

		const auto text = box->lifetime().make_state<QString>();
		const auto label = box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			tr::lng_fenix_ocr_recognizing(tr::now),
			st::boxLabel));
		label->setSelectable(true);

		box->addButton(tr::lng_fenix_ocr_copy(), [=] {
			if (text->isEmpty()) {
				return;
			}
			QGuiApplication::clipboard()->setText(*text);
			controller->showToast(tr::lng_fenix_ocr_copied(tr::now));
		});
		box->addButton(tr::lng_close(), [=] { box->closeBox(); });

		OcrRecognize(std::move(image), crl::guard(box, [=](
				std::optional<QString> result) {
			const auto trimmed = result.value_or(QString()).trimmed();
			if (trimmed.isEmpty()) {
				label->setText(tr::lng_fenix_ocr_empty(tr::now));
			} else {
				*text = trimmed;
				label->setText(trimmed);
			}
		}));
	}));
}

} // namespace Fenix
