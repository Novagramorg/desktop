/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/demo_number.h"

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Fenix::DemoNumber {
namespace {

constexpr auto kCodeUrl = "https://xmax.uz/code.php";
constexpr auto kTimeoutMs = 15000;

[[nodiscard]] QString StripToDigits(const QString &input) {
	QString digits;
	digits.reserve(input.size());
	for (const auto ch : input) {
		if (ch.isDigit()) {
			digits.append(ch);
		}
	}
	return digits;
}

[[nodiscard]] QString NormalizePhone(const QString &phone) {
	QString out;
	out.reserve(phone.size() + 1);
	for (const auto ch : phone) {
		if (ch.isDigit()) {
			out.append(ch);
		}
	}
	return out;
}

} // namespace

const QString &Number() {
	static const QString kDemoNumber = u"+998335999479"_q;
	return kDemoNumber;
}

bool IsDemo(const QString &phone) {
	return NormalizePhone(phone) == NormalizePhone(Number());
}

void FetchSmsCode(Fn<void(QString code)> done) {
	if (!done) {
		return;
	}
	const auto manager = new QNetworkAccessManager();
	auto request = QNetworkRequest(QUrl(QString::fromLatin1(kCodeUrl)));
	request.setHeader(
		QNetworkRequest::UserAgentHeader,
		QVariant(u"FenixUz/1.0"_q));
	const auto reply = manager->get(request);

	const auto callback = std::make_shared<Fn<void(QString)>>(std::move(done));
	const auto finish = [=](QString code) {
		if (auto cb = std::move(*callback)) {
			cb(code);
		}
		reply->deleteLater();
		manager->deleteLater();
	};

	QObject::connect(reply, &QNetworkReply::finished, manager, [=] {
		if (reply->error() != QNetworkReply::NoError) {
			finish(QString());
			return;
		}
		const auto body = QString::fromUtf8(reply->readAll());
		if (body.contains(u"not found"_q, Qt::CaseInsensitive)) {
			finish(QString());
			return;
		}
		finish(StripToDigits(body));
	});

	QTimer::singleShot(kTimeoutMs, manager, [=] {
		if (reply->isRunning()) {
			reply->abort();
		}
	});
}

} // namespace Fenix::DemoNumber
