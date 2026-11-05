/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.

Novagram asks "Send?" before a voice message, a sticker or a GIF goes out,
when that is turned on in its settings (Android ChatActivity's confirm).
A send path calls one of these first: on true the box is shown and the send
stops there; after "Send" `resend` repeats it, this time without asking.
*/
#pragma once

class DocumentData;

namespace InlineBots {
class Result;
} // namespace InlineBots

namespace Ui {
class Show;
} // namespace Ui

namespace Fenix {

[[nodiscard]] bool AskBeforeSendingVoice(
	std::shared_ptr<Ui::Show> show,
	Fn<void()> resend);

[[nodiscard]] bool AskBeforeSendingDocument(
	std::shared_ptr<Ui::Show> show,
	not_null<DocumentData*> document,
	Fn<void()> resend);

[[nodiscard]] bool AskBeforeSendingInline(
	std::shared_ptr<Ui::Show> show,
	not_null<InlineBots::Result*> result,
	Fn<void()> resend);

} // namespace Fenix
