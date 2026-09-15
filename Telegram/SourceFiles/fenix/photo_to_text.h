/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

"Scan text" (OCR): recognise the text inside a photo and show it for copying.
Port of the Android org.fenixuz.ui.photo_to_text.PhotoToText. On Android this
used ML Kit; on desktop the recognition runs on-device through the Windows OCR
engine (winrt Windows.Media.Ocr) — no network, no extra downloads. On platforms
without a built-in OCR engine the feature reports "not available".
*/
#pragma once

#include <optional>

class QImage;

namespace Window {
class SessionController;
} // namespace Window

namespace Fenix {

// Whether on-device OCR is available on this platform/build.
[[nodiscard]] bool OcrSupported();

// Recognise text in [image] off the UI flow; [done] is always invoked on the
// main thread, with std::nullopt on failure / no engine.
void OcrRecognize(QImage image, Fn<void(std::optional<QString>)> done);

// Open the "Scan text" box for an already-decoded image and run OCR into it.
void ShowPhotoToText(
	not_null<Window::SessionController*> controller,
	QImage image);

} // namespace Fenix
