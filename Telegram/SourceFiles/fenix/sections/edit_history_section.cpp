/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/sections/edit_history_section.h"

#include "fenix/edit_history.h"
#include "window/section_widget.h"
#include "window/section_memento.h"
#include "window/window_session_controller.h"
#include "data/data_peer.h"
#include "lang/lang_keys.h"
#include "base/unixtime.h"
#include "ui/widgets/scroll_area.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/shadow.h"
#include "ui/text/text.h"
#include "ui/text/text_options.h"
#include "ui/chat/chat_theme.h"
#include "ui/painter.h"
#include "ui/rect.h"
#include "ui/ui_utility.h"
#include "styles/style_info.h"
#include "styles/style_chat.h"

#include <QtCore/QLocale>

namespace Fenix {
namespace {

// Inner scrollable widget that paints every saved version as a chat-style
// incoming bubble, with its edit time in the bottom-right corner.
class HistoryInner final : public Ui::RpWidget {
public:
	HistoryInner(
		QWidget *parent,
		std::vector<EditHistory::Version> versions);

protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent *e) override;

private:
	struct Entry {
		Ui::Text::String text;
		QString date;
		int dateWidth = 0;
		int contentWidth = 0;
		QRect bubble;
	};
	std::vector<Entry> _entries;

};

HistoryInner::HistoryInner(
	QWidget *parent,
	std::vector<EditHistory::Version> versions)
: RpWidget(parent) {
	_entries.reserve(versions.size());
	for (const auto &version : versions) {
		auto entry = Entry();
		entry.text = Ui::Text::String(st::msgMinWidth);
		entry.text.setText(
			st::messageTextStyle,
			version.text.isEmpty()
				? QString::fromUtf8("\xE2\x80\x94")
				: version.text,
			Ui::ItemTextDefaultOptions());
		const auto when = base::unixtime::parse(version.date);
		entry.date = QLocale().toString(when, QLocale::ShortFormat);
		entry.dateWidth = st::msgDateFont->width(entry.date);
		_entries.push_back(std::move(entry));
	}
}

int HistoryInner::resizeGetHeight(int newWidth) {
	const auto pad = st::msgPadding;
	const auto leftMargin = st::msgMargin.left();
	const auto rightSkip = st::msgMargin.right();
	const auto available = newWidth - leftMargin - rightSkip;
	const auto maxContent = std::min(available, int(st::msgMaxWidth))
		- pad.left()
		- pad.right();
	const auto dateGap = 2;
	const auto dateHeight = st::msgDateFont->height;

	auto top = st::msgMargin.top();
	for (auto &entry : _entries) {
		const auto natural = entry.text.maxWidth();
		auto contentWidth = std::min(natural, std::max(maxContent, 1));
		contentWidth = std::max(contentWidth, entry.dateWidth);
		entry.contentWidth = contentWidth;

		const auto textHeight = entry.text.countHeight(contentWidth);
		const auto bubbleWidth = contentWidth + pad.left() + pad.right();
		const auto bubbleHeight = pad.top()
			+ textHeight
			+ dateGap
			+ dateHeight
			+ pad.bottom();
		entry.bubble = QRect(leftMargin, top, bubbleWidth, bubbleHeight);
		top += bubbleHeight + st::msgMargin.top() + st::msgMargin.bottom();
	}
	return std::max(top, st::msgMargin.top());
}

void HistoryInner::paintEvent(QPaintEvent *e) {
	auto p = Painter(this);
	const auto clip = e->rect();
	const auto pad = st::msgPadding;
	const auto radius = st::bubbleRadiusLarge;
	const auto dateHeight = st::msgDateFont->height;

	for (const auto &entry : _entries) {
		const auto &bubble = entry.bubble;
		if (!bubble.intersects(clip)) {
			continue;
		}
		{
			auto hq = PainterHighQualityEnabler(p);
			p.setPen(Qt::NoPen);
			p.setBrush(st::msgInBg);
			p.drawRoundedRect(bubble, radius, radius);
		}

		p.setPen(st::historyTextInFg);
		entry.text.draw(
			p,
			bubble.x() + pad.left(),
			bubble.y() + pad.top(),
			entry.contentWidth);

		p.setFont(st::msgDateFont);
		p.setPen(st::msgInDateFg);
		p.drawText(
			bubble.x() + bubble.width() - pad.right() - entry.dateWidth,
			bubble.y() + bubble.height() - pad.bottom() - dateHeight
				+ st::msgDateFont->ascent,
			entry.date);
	}
}

class Widget;

class Memento final : public Window::SectionMemento {
public:
	Memento(not_null<PeerData*> peer, MsgId msgId)
	: _peer(peer)
	, _msgId(msgId) {
	}

	object_ptr<Window::SectionWidget> createWidget(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		Window::Column column,
		const QRect &geometry) override;

	[[nodiscard]] not_null<PeerData*> peer() const {
		return _peer;
	}
	[[nodiscard]] MsgId msgId() const {
		return _msgId;
	}

private:
	const not_null<PeerData*> _peer;
	const MsgId _msgId;

};

class Widget final : public Window::SectionWidget {
public:
	Widget(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer,
		MsgId msgId);

	bool showInternal(
		not_null<Window::SectionMemento*> memento,
		const Window::SectionShow &params) override;

	bool hasTopBarShadow() const override {
		return true;
	}

	QRect floatPlayerAvailableRect() override {
		return mapToGlobal(_scroll->geometry());
	}
	bool floatPlayerHandleWheelEvent(QEvent *e) override {
		return false;
	}

protected:
	void resizeEvent(QResizeEvent *e) override;
	void paintEvent(QPaintEvent *e) override;

private:
	void setupTopBar();

	not_null<PeerData*> _peer;
	MsgId _msgId = 0;

	object_ptr<Ui::RpWidget> _topBar = { nullptr };
	object_ptr<Ui::IconButton> _back = { nullptr };
	object_ptr<Ui::FlatLabel> _title = { nullptr };
	object_ptr<Ui::PlainShadow> _topShadow = { nullptr };
	std::unique_ptr<Ui::ScrollArea> _scroll;
	QPointer<HistoryInner> _inner;
	std::shared_ptr<Ui::ChatTheme> _theme;

};

Widget::Widget(
	QWidget *parent,
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer,
	MsgId msgId)
: Window::SectionWidget(parent, controller, rpl::single<PeerData*>(peer.get()))
, _peer(peer)
, _msgId(msgId)
, _topBar(this)
, _back(this, st::infoTopBarBack)
, _title(this, st::infoTopBarTitle)
, _topShadow(this)
, _scroll(std::make_unique<Ui::ScrollArea>(this, st::historyScroll)) {
	Window::ChatThemeValueFromPeer(
		controller,
		peer
	) | rpl::on_next([=](std::shared_ptr<Ui::ChatTheme> &&theme) {
		_theme = std::move(theme);
		controller->setChatStyleTheme(_theme);
		update();
	}, lifetime());

	setupTopBar();

	_inner = _scroll->setOwnedWidget(object_ptr<HistoryInner>(
		this,
		GetEdits().versions(peer->id, msgId)));
	_inner->show();
	_scroll->show();
}

void Widget::setupTopBar() {
	_topBar->paintRequest(
	) | rpl::on_next([=](QRect clip) {
		auto p = QPainter(_topBar.get());
		p.fillRect(clip, st::windowBg);
	}, _topBar->lifetime());

	_title->setText(_peer->name());
	_title->setAttribute(Qt::WA_TransparentForMouseEvents);

	_back->setClickedCallback([=] {
		controller()->showBackFromStack();
	});
}

bool Widget::showInternal(
		not_null<Window::SectionMemento*> memento,
		const Window::SectionShow &params) {
	if (const auto my = dynamic_cast<Memento*>(memento.get())) {
		if (my->peer() == _peer && my->msgId() == _msgId) {
			return true;
		}
	}
	return false;
}

void Widget::paintEvent(QPaintEvent *e) {
	SectionWidget::paintEvent(e);
	if (animatingShow()) {
		return;
	} else if (_theme) {
		SectionWidget::PaintBackground(
			controller(),
			_theme.get(),
			this,
			e->rect());
	} else {
		QPainter(this).fillRect(e->rect(), st::windowBg);
	}
}

void Widget::resizeEvent(QResizeEvent *e) {
	const auto topBarHeight = st::infoTopBarHeight;
	_topBar->setGeometry(0, 0, width(), topBarHeight);

	const auto backWidth = _back->width();
	_back->moveToLeft(0, 0);
	_title->resizeToWidth(std::max(1, width() - backWidth - topBarHeight));
	_title->moveToLeft(
		backWidth,
		(topBarHeight - _title->height()) / 2);

	_topShadow->setGeometry(0, topBarHeight, width(), st::lineWidth);

	const auto scrollTop = topBarHeight;
	_scroll->setGeometry(0, scrollTop, width(), height() - scrollTop);
	if (_inner) {
		_inner->resizeToWidth(_scroll->width());
	}
}

object_ptr<Window::SectionWidget> Memento::createWidget(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		Window::Column column,
		const QRect &geometry) {
	auto result = object_ptr<Widget>(
		parent,
		controller,
		_peer,
		_msgId);
	result->setGeometry(geometry);
	return result;
}

} // namespace

void ShowEditHistory(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer,
		MsgId msgId) {
	controller->showSection(std::make_shared<Memento>(peer, msgId));
}

} // namespace Fenix
