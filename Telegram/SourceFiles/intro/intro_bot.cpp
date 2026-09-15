/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "intro/intro_bot.h"

#include "lang/lang_keys.h"
#include "styles/style_intro.h"
#include "ui/widgets/fields/input_field.h"
#include "config.h"

#include <QtCore/QRegularExpression>

namespace Intro {
namespace details {
namespace {

// BotFather tokens look like <bot_user_id>:<secret>, e.g. 123456789:AAExxx...
[[nodiscard]] bool IsValidToken(const QString &token) {
	static const auto kRegExp = QRegularExpression(
		u"^\\d{5,}:[A-Za-z0-9_-]{30,}$"_q);
	return kRegExp.match(token).hasMatch();
}

} // namespace

BotWidget::BotWidget(
	QWidget *parent,
	not_null<Main::Account*> account,
	not_null<Data*> data)
: Step(parent, account, data)
, _token(this, st::introName, tr::lng_fenix_bot_token()) {
	_token->setAccessibleName(tr::lng_fenix_bot_token(tr::now));

	_token->submits(
	) | rpl::on_next([=](Qt::KeyboardModifiers) {
		submit();
	}, _token->lifetime());
	_token->changes(
	) | rpl::on_next([=] {
		hideError();
	}, _token->lifetime());

	setTitleText(tr::lng_fenix_bot_login());
	setDescriptionText(tr::lng_fenix_bot_desc());
	setErrorCentered(true);
}

rpl::producer<QString> BotWidget::nextButtonText() const {
	return tr::lng_fenix_bot_login();
}

void BotWidget::resizeEvent(QResizeEvent *e) {
	Step::resizeEvent(e);
	_token->moveToLeft(
		contentLeft(),
		contentTop() + st::introStepFieldTop);
}

void BotWidget::submit() {
	if (_sentRequest || isHidden()) {
		return;
	}
	const auto token = _token->getLastText().trimmed();
	if (!IsValidToken(token)) {
		_token->showError();
		showError(tr::lng_fenix_bot_invalid());
		return;
	}
	hideError();
	cancelNearestDcRequest();

	_sentRequest = api().request(MTPauth_ImportBotAuthorization(
		MTP_int(0),
		MTP_int(ApiId),
		MTP_string(ApiHash),
		MTP_string(token)
	)).done([=](const MTPauth_Authorization &result) {
		requestDone(result);
	}).fail([=](const MTP::Error &error) {
		requestFail(error);
	}).handleFloodErrors().send();
}

void BotWidget::requestDone(const MTPauth_Authorization &result) {
	_sentRequest = 0;
	// Same completion path as QR / phone login (session setup, persistence).
	finish(result);
}

void BotWidget::requestFail(const MTP::Error &error) {
	_sentRequest = 0;
	if (error.type().startsWith(u"FLOOD_WAIT"_q)) {
		showError(tr::lng_flood_error());
		return;
	}
	// ACCESS_TOKEN_INVALID / ACCESS_TOKEN_EXPIRED / a malformed token, etc.
	showError(tr::lng_fenix_bot_invalid());
}

void BotWidget::setInnerFocus() {
	_token->setFocusFast();
}

void BotWidget::activate() {
	Step::activate();
	showChildren();
	setInnerFocus();
}

void BotWidget::cancelled() {
	api().request(base::take(_sentRequest)).cancel();
}

void BotWidget::finished() {
	Step::finished();
	apiClear();
	cancelled();
}

} // namespace details
} // namespace Intro
