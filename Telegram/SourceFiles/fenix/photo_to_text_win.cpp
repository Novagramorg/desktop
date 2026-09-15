/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Windows on-device OCR backend for "Scan text", via winrt Windows.Media.Ocr.
No network, no extra downloads — uses the OCR language packs already on the
machine. Compiled only on Windows (see the _win suffix).
*/
#include "fenix/photo_to_text.h"

#include "base/platform/win/base_windows_winrt.h"

#include <QtGui/QImage>

#include <algorithm>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.Streams.h>

namespace Fenix {

bool OcrSupported() {
	return base::WinRT::Try([&] {
		return winrt::Windows::Media::Ocr::OcrEngine
			::AvailableRecognizerLanguages().Size() > 0;
	}).value_or(false);
}

void OcrRecognize(QImage image, Fn<void(std::optional<QString>)> done) {
	using namespace winrt::Windows::Foundation;
	using namespace winrt::Windows::Graphics::Imaging;
	using namespace winrt::Windows::Media::Ocr;
	using namespace winrt::Windows::Security::Cryptography;

	const auto callback = std::make_shared<Fn<void(std::optional<QString>)>>(
		std::move(done));
	const auto report = [=](std::optional<QString> value) {
		crl::on_main([=] {
			if (*callback) {
				(*callback)(value);
			}
		});
	};

	const auto ok = base::WinRT::Try([&] {
		// Downsample very large images: keeps the main-thread pixel copy cheap
		// and stays within the OCR engine's input limits.
		constexpr auto kMaxSide = 2560;
		auto source = image.convertToFormat(QImage::Format_ARGB32);
		const auto side = std::max(source.width(), source.height());
		if (side > kMaxSide) {
			source = source.scaled(
				source.width() * kMaxSide / side,
				source.height() * kMaxSide / side,
				Qt::IgnoreAspectRatio,
				Qt::SmoothTransformation
			).convertToFormat(QImage::Format_ARGB32);
		}
		const auto width = source.width();
		const auto height = source.height();
		if (width <= 0 || height <= 0) {
			report(std::nullopt);
			return;
		}
		// QImage Format_ARGB32 keeps pixels as 0xAARRGGBB; on little-endian that
		// is B,G,R,A in memory — exactly Bgra8. Rows are tightly packed (4 bytes
		// per pixel, already 4-byte aligned), so the buffer maps 1:1.
		const auto first = reinterpret_cast<const uint8_t*>(source.constBits());
		const auto bytes = winrt::array_view<const uint8_t>(
			first,
			first + (std::ptrdiff_t(width) * height * 4));
		const auto buffer = CryptographicBuffer::CreateFromByteArray(bytes);
		const auto bitmap = SoftwareBitmap::CreateCopyFromBuffer(
			buffer,
			BitmapPixelFormat::Bgra8,
			width,
			height,
			BitmapAlphaMode::Ignore);

		auto engine = OcrEngine::TryCreateFromUserProfileLanguages();
		if (!engine) {
			const auto langs = OcrEngine::AvailableRecognizerLanguages();
			if (langs.Size() > 0) {
				engine = OcrEngine::TryCreateFromLanguage(langs.GetAt(0));
			}
		}
		if (!engine) {
			report(std::nullopt);
			return;
		}
		engine.RecognizeAsync(bitmap).Completed([=](
				IAsyncOperation<OcrResult> that,
				AsyncStatus status) {
			if (status != AsyncStatus::Completed) {
				report(std::nullopt);
				return;
			}
			const auto text = base::WinRT::Try([&]() -> QString {
				const auto result = that.GetResults();
				const auto hstr = result.Text();
				return QString::fromWCharArray(hstr.c_str(), int(hstr.size()));
			});
			report(text ? std::make_optional(*text) : std::nullopt);
		});
	});
	if (!ok) {
		report(std::nullopt);
	}
}

} // namespace Fenix
