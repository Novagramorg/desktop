/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/confirm_send.h"

#include "data/data_document.h"
#include "fenix/fenix_settings.h"
#include "inline_bots/inline_bot_result.h"
#include "lang/lang_keys.h"
#include "ui/boxes/confirm_box.h"
#include "ui/layers/show.h"

namespace Fenix {
namespace {

// While a confirmed send is repeated, nothing asks again.
auto Confirmed = 0;

bool Ask(
		std::shared_ptr<Ui::Show> show,
		const QString &text,
		Fn<void()> resend) {
	if (Confirmed > 0) {
		return false;
	}
	show->showBox(Ui::MakeConfirmBox({
		.text = text,
		.confirmed = [=](Fn<void()> close) {
			// The box goes away with its callbacks: keep our own copy.
			const auto send = resend;
			close();

			++Confirmed;
			const auto guard = gsl::finally([] { --Confirmed; });
			send();
		},
		.confirmText = tr::lng_send_button(),
	}));
	return true;
}

} // namespace

bool AskBeforeSendingVoice(
		std::shared_ptr<Ui::Show> show,
		Fn<void()> resend) {
	return Get().confirmVoice()
		&& Ask(show, tr::lng_fenix_confirm_voice(tr::now), std::move(resend));
}

bool AskBeforeSendingDocument(
		std::shared_ptr<Ui::Show> show,
		not_null<DocumentData*> document,
		Fn<void()> resend) {
	if (document->sticker() && Get().confirmSticker()) {
		return Ask(
			show,
			tr::lng_fenix_confirm_sticker(tr::now),
			std::move(resend));
	} else if (document->isGifv() && Get().confirmGif()) {
		return Ask(
			show,
			tr::lng_fenix_confirm_gif(tr::now),
			std::move(resend));
	}
	return false;
}

bool AskBeforeSendingInline(
		std::shared_ptr<Ui::Show> show,
		not_null<InlineBots::Result*> result,
		Fn<void()> resend) {
	return result->isGif()
		&& Get().confirmGif()
		&& Ask(show, tr::lng_fenix_confirm_gif(tr::now), std::move(resend));
}

} // namespace Fenix
