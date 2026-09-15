/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#pragma once

#include "intro/intro_step.h"

namespace Ui {
class InputField;
} // namespace Ui

namespace Intro {
namespace details {

// Novagram: log in as a bot using a @BotFather token instead of a phone number,
// via the MTProto auth.importBotAuthorization method. The resulting
// auth.Authorization is handed to Step::finish, so the bot account goes through
// the exact same session setup as a normal login.
class BotWidget final : public Step {
public:
	BotWidget(
		QWidget *parent,
		not_null<Main::Account*> account,
		not_null<Data*> data);

	void setInnerFocus() override;
	void activate() override;
	void cancelled() override;
	void finished() override;
	void submit() override;

	[[nodiscard]] rpl::producer<QString> nextButtonText() const override;

	bool hasBack() const override {
		return true;
	}

protected:
	void resizeEvent(QResizeEvent *e) override;

private:
	void requestDone(const MTPauth_Authorization &result);
	void requestFail(const MTP::Error &error);

	object_ptr<Ui::InputField> _token;
	mtpRequestId _sentRequest = 0;

};

} // namespace details
} // namespace Intro
