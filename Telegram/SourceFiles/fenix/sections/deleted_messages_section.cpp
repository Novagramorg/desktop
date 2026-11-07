/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/sections/deleted_messages_section.h"

#include "base/unique_qptr.h"
#include "base/unixtime.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "fenix/kept_messages.h"
#include "fenix/saved_history.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/view/history_view_item_preview.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "ui/boxes/confirm_box.h"
#include "ui/chat/chat_theme.h"
#include "ui/painter.h"
#include "ui/text/text_options.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/popup_menu.h"
#include "ui/widgets/scroll_area.h"
#include "ui/widgets/shadow.h"
#include "window/section_memento.h"
#include "window/section_widget.h"
#include "window/window_session_controller.h"
#include "styles/style_chat.h"
#include "styles/style_fenix.h"
#include "styles/style_info.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_widgets.h"

#include <QtCore/QLocale>
#include <QtGui/QClipboard>
#include <QtGui/QCursor>
#include <QtGui/QGuiApplication>

namespace Fenix {
namespace {

struct Entry {
	MsgId id = 0;
	QString name;
	Ui::Text::String text;
	QString plain;
	QString date;
	int nameWidth = 0;
	int dateWidth = 0;
	int contentWidth = 0;
	int textHeight = 0;
	QRect bubble;
};

[[nodiscard]] std::vector<Entry> CollectEntries(not_null<History*> history) {
	// A copy: creating the items doesn't change the list, but stays safe.
	const auto list = GetKept(&history->session()).list(history->peer->id);
	auto result = std::vector<Entry>();
	result.reserve(list.size());
	for (const auto &kept : ranges::views::reverse(list)) {
		const auto item = KeptHistory::Item(history, kept);
		if (!item) {
			continue;
		}
		// A media message shows its kind ("Photo", "Voice message"...) and
		// its caption, as the chats list does.
		auto text = item->media()
			? item->toPreview({
				.hideSender = true,
				.generateImages = false,
			}).text
			: item->originalText();
		auto entry = Entry{ .id = kept.id };
		entry.name = item->from()->name();
		entry.plain = text.text;
		if (text.text.isEmpty()) {
			text = TextWithEntities{ QString::fromUtf8("\xE2\x80\x94") };
		}
		entry.text = Ui::Text::String(st::msgMinWidth);
		entry.text.setMarkedText(
			st::messageTextStyle,
			text,
			Ui::ItemTextDefaultOptions());
		entry.date = QLocale().toString(
			base::unixtime::parse(item->date()),
			QLocale::ShortFormat);
		entry.nameWidth = st::msgNameFont->width(entry.name);
		entry.dateWidth = st::msgDateFont->width(entry.date);
		result.push_back(std::move(entry));
	}
	return result;
}

class Inner final : public Ui::RpWidget {
public:
	Inner(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		not_null<History*> history);

	[[nodiscard]] bool empty() const;
	[[nodiscard]] rpl::producer<bool> emptyValue() const;
	void setViewportHeight(int height);

protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void contextMenuEvent(QContextMenuEvent *e) override;
	void leaveEventHook(QEvent *e) override;

private:
	void refresh();
	void paintEmpty(Painter &p);
	void select(int index);
	[[nodiscard]] int entryAt(QPoint position) const;
	void showMenu(int index, QPoint globalPosition);
	void confirmForget(MsgId id);

	const not_null<Window::SessionController*> _controller;
	const not_null<History*> _history;
	std::vector<Entry> _entries;
	Ui::Text::String _emptyAbout;
	QRect _emptyRect;
	int _viewportHeight = 0;
	int _selected = -1;
	base::unique_qptr<Ui::PopupMenu> _menu;
	rpl::variable<bool> _empty = true;

};

Inner::Inner(
	QWidget *parent,
	not_null<Window::SessionController*> controller,
	not_null<History*> history)
: RpWidget(parent)
, _controller(controller)
, _history(history)
, _emptyAbout(st::novagramDeletedEmptyWidth) {
	setMouseTracking(true);
	_emptyAbout.setText(
		st::defaultTextStyle,
		tr::lng_fenix_deleted_messages_empty_about(tr::now),
		Ui::NameTextOptions());

	GetKept(&history->session()).changes(
	) | rpl::filter([=](PeerId peerId) {
		return (peerId == history->peer->id);
	}) | rpl::on_next([=] {
		refresh();
	}, lifetime());

	refresh();
}

bool Inner::empty() const {
	return _empty.current();
}

rpl::producer<bool> Inner::emptyValue() const {
	return _empty.value();
}

void Inner::setViewportHeight(int height) {
	if (_viewportHeight != height) {
		_viewportHeight = height;
		resizeToWidth(width());
	}
}

void Inner::refresh() {
	_entries = CollectEntries(_history);
	_selected = -1;
	setCursor(style::cur_default);
	_empty = _entries.empty();
	resizeToWidth(width());
	update();
}

int Inner::resizeGetHeight(int newWidth) {
	const auto pad = st::msgPadding;
	const auto margin = st::msgMargin;
	if (_entries.empty()) {
		const auto &epad = st::novagramDeletedEmptyPadding;
		const auto inner = std::min(
			int(st::novagramDeletedEmptyWidth),
			newWidth - margin.left() - margin.right()
				- epad.left() - epad.right());
		const auto width = std::max(inner, 1);
		const auto height = epad.top()
			+ st::novagramDeletedEmptyTitleFont->height
			+ st::novagramDeletedEmptySkip
			+ _emptyAbout.countHeight(width)
			+ epad.bottom();
		const auto full = width + epad.left() + epad.right();
		const auto outer = std::max(
			_viewportHeight,
			height + margin.top() + margin.bottom());
		_emptyRect = QRect(
			(newWidth - full) / 2,
			(outer - height) / 2,
			full,
			height);
		return outer;
	}

	const auto available = newWidth - margin.left() - margin.right();
	const auto maxContent = std::max(
		std::min(available, int(st::msgMaxWidth))
			- pad.left()
			- pad.right(),
		1);
	auto top = margin.top();
	for (auto &entry : _entries) {
		auto content = std::min(entry.text.maxWidth(), maxContent);
		content = std::max({ content, entry.dateWidth, 1 });
		content = std::max(content, std::min(entry.nameWidth, maxContent));
		entry.contentWidth = content;
		entry.textHeight = entry.text.countHeight(content);
		const auto height = pad.top()
			+ st::msgNameFont->height
			+ st::novagramDeletedNameSkip
			+ entry.textHeight
			+ st::novagramDeletedDateSkip
			+ st::msgDateFont->height
			+ pad.bottom();
		entry.bubble = QRect(
			margin.left(),
			top,
			content + pad.left() + pad.right(),
			height);
		top += height + margin.top() + margin.bottom();
	}
	return std::max(top, _viewportHeight);
}

void Inner::paintEvent(QPaintEvent *e) {
	auto p = Painter(this);
	if (_entries.empty()) {
		paintEmpty(p);
		return;
	}
	const auto clip = e->rect();
	const auto pad = st::msgPadding;
	const auto radius = st::bubbleRadiusLarge;
	for (auto i = 0, count = int(_entries.size()); i != count; ++i) {
		const auto &entry = _entries[i];
		const auto &bubble = entry.bubble;
		if (!bubble.intersects(clip)) {
			continue;
		}
		{
			auto hq = PainterHighQualityEnabler(p);
			p.setPen(Qt::NoPen);
			p.setBrush((i == _selected) ? st::msgInBgSelected : st::msgInBg);
			p.drawRoundedRect(bubble, radius, radius);
		}
		const auto left = bubble.x() + pad.left();
		auto top = bubble.y() + pad.top();

		p.setFont(st::msgNameFont);
		p.setPen(st::msgInServiceFg);
		p.drawTextLeft(
			left,
			top,
			width(),
			st::msgNameFont->elided(entry.name, entry.contentWidth));
		top += st::msgNameFont->height + st::novagramDeletedNameSkip;

		p.setPen(st::historyTextInFg);
		entry.text.draw(p, left, top, entry.contentWidth);
		top += entry.textHeight + st::novagramDeletedDateSkip;

		p.setFont(st::msgDateFont);
		p.setPen(st::msgInDateFg);
		p.drawTextLeft(
			bubble.x() + bubble.width() - pad.right() - entry.dateWidth,
			top,
			width(),
			entry.date);
	}
}

void Inner::paintEmpty(Painter &p) {
	const auto &epad = st::novagramDeletedEmptyPadding;
	{
		auto hq = PainterHighQualityEnabler(p);
		p.setPen(Qt::NoPen);
		p.setBrush(st::msgServiceBg);
		p.drawRoundedRect(
			_emptyRect,
			st::bubbleRadiusLarge,
			st::bubbleRadiusLarge);
	}
	const auto inner = _emptyRect.marginsRemoved(epad);
	const auto font = st::novagramDeletedEmptyTitleFont;
	const auto title = font->elided(
		tr::lng_fenix_deleted_messages_empty(tr::now),
		inner.width());
	p.setFont(font);
	p.setPen(st::msgServiceFg);
	p.drawText(
		inner.x() + (inner.width() - font->width(title)) / 2,
		inner.y() + font->ascent,
		title);
	_emptyAbout.draw(p, {
		.position = QPoint(
			inner.x(),
			inner.y() + font->height + st::novagramDeletedEmptySkip),
		.availableWidth = inner.width(),
		.align = style::al_top,
	});
}

int Inner::entryAt(QPoint position) const {
	for (auto i = 0, count = int(_entries.size()); i != count; ++i) {
		if (_entries[i].bubble.contains(position)) {
			return i;
		}
	}
	return -1;
}

void Inner::select(int index) {
	if (_selected != index) {
		_selected = index;
		setCursor((index >= 0) ? style::cur_pointer : style::cur_default);
		update();
	}
}

void Inner::mouseMoveEvent(QMouseEvent *e) {
	select(entryAt(e->pos()));
}

void Inner::leaveEventHook(QEvent *e) {
	if (!_menu) {
		select(-1);
	}
}

void Inner::mousePressEvent(QMouseEvent *e) {
	if (e->button() != Qt::LeftButton) {
		return;
	} else if (const auto index = entryAt(e->pos()); index >= 0) {
		showMenu(index, e->globalPos());
	}
}

void Inner::contextMenuEvent(QContextMenuEvent *e) {
	const auto index = (e->reason() == QContextMenuEvent::Mouse)
		? entryAt(e->pos())
		: _selected;
	if (index >= 0) {
		showMenu(index, e->globalPos());
	}
}

void Inner::showMenu(int index, QPoint globalPosition) {
	const auto id = _entries[index].id;
	const auto plain = _entries[index].plain;
	select(index);
	_menu = base::make_unique_q<Ui::PopupMenu>(
		this,
		st::popupMenuWithIcons);
	if (!plain.isEmpty()) {
		_menu->addAction(tr::lng_context_copy_text(tr::now), [=] {
			QGuiApplication::clipboard()->setText(plain);
			_controller->showToast(tr::lng_text_copied(tr::now));
		}, &st::menuIconCopy);
	}
	_menu->addAction(tr::lng_fenix_deleted_messages_forget(tr::now), [=] {
		confirmForget(id);
	}, &st::menuIconDelete);
	_menu->setDestroyedCallback(crl::guard(this, [=] {
		select(entryAt(mapFromGlobal(QCursor::pos())));
	}));
	_menu->popup(globalPosition);
}

void Inner::confirmForget(MsgId id) {
	const auto history = _history;
	_controller->show(Ui::MakeConfirmBox({
		.text = tr::lng_fenix_deleted_messages_forget_sure(),
		.confirmed = [=](Fn<void()> close) {
			ForgetKeptMessage(history, id);
			close();
		},
		.confirmText = tr::lng_box_delete(),
		.confirmStyle = &st::attentionBoxButton,
	}));
}

class Memento final : public Window::SectionMemento {
public:
	explicit Memento(not_null<PeerData*> peer) : _peer(peer) {
	}

	object_ptr<Window::SectionWidget> createWidget(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		Window::Column column,
		const QRect &geometry) override;

	[[nodiscard]] not_null<PeerData*> peer() const {
		return _peer;
	}

private:
	const not_null<PeerData*> _peer;

};

class Widget final : public Window::SectionWidget {
public:
	Widget(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer);

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
	void showFinishedHook() override;

private:
	void setupTopBar();
	void confirmClear();
	void updateClearVisibility();

	const not_null<PeerData*> _peer;
	const not_null<History*> _history;

	object_ptr<Ui::RpWidget> _topBar;
	object_ptr<Ui::IconButton> _back;
	object_ptr<Ui::FlatLabel> _title;
	object_ptr<Ui::FlatLabel> _subtitle;
	object_ptr<Ui::IconButton> _clear;
	object_ptr<Ui::PlainShadow> _topShadow;
	std::unique_ptr<Ui::ScrollArea> _scroll;
	QPointer<Inner> _inner;
	std::shared_ptr<Ui::ChatTheme> _theme;

};

Widget::Widget(
	QWidget *parent,
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer)
: Window::SectionWidget(parent, controller, rpl::single<PeerData*>(peer.get()))
, _peer(peer)
, _history(peer->owner().history(peer))
, _topBar(this)
, _back(this, st::infoTopBarBack)
, _title(this, tr::lng_fenix_deleted_messages(), st::infoTopBarTitle)
, _subtitle(this, st::novagramDeletedSubtitle)
, _clear(_topBar.data(), st::novagramDeletedClear)
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

	_inner = _scroll->setOwnedWidget(
		object_ptr<Inner>(this, controller, _history));
	_inner->emptyValue(
	) | rpl::on_next([=] {
		updateClearVisibility();
	}, _inner->lifetime());
	_inner->show();
	_scroll->show();
}

void Widget::setupTopBar() {
	_topBar->paintRequest(
	) | rpl::on_next([=](QRect clip) {
		auto p = QPainter(_topBar.get());
		p.fillRect(clip, st::windowBg);
	}, _topBar->lifetime());

	_title->setAttribute(Qt::WA_TransparentForMouseEvents);
	_subtitle->setAttribute(Qt::WA_TransparentForMouseEvents);
	_subtitle->setText(_peer->name());

	_back->setClickedCallback([=] {
		controller()->showBackFromStack();
	});
	_clear->setClickedCallback([=] {
		confirmClear();
	});
}

void Widget::updateClearVisibility() {
	_clear->setVisible(_inner && !_inner->empty());
}

void Widget::showFinishedHook() {
	// The show animation shows all the children again.
	updateClearVisibility();
}

void Widget::confirmClear() {
	const auto history = _history;
	controller()->show(Ui::MakeConfirmBox({
		.text = tr::lng_fenix_deleted_messages_clear_sure(),
		.confirmed = [=](Fn<void()> close) {
			ForgetKeptChatMessages(history);
			close();
		},
		.confirmText = tr::lng_fenix_deleted_messages_clear(),
		.confirmStyle = &st::attentionBoxButton,
	}));
}

bool Widget::showInternal(
		not_null<Window::SectionMemento*> memento,
		const Window::SectionShow &params) {
	if (const auto my = dynamic_cast<Memento*>(memento.get())) {
		return (my->peer() == _peer);
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

	_back->moveToLeft(0, 0);
	_clear->moveToRight(0, 0);
	const auto left = _back->width();
	const auto right = _clear->width();
	const auto available = std::max(1, width() - left - right);
	_title->resizeToWidth(available);
	_subtitle->resizeToWidth(available);
	const auto textHeight = _title->height() + _subtitle->height();
	const auto top = (topBarHeight - textHeight) / 2;
	_title->moveToLeft(left, top);
	_subtitle->moveToLeft(left, top + _title->height());

	_topShadow->setGeometry(0, topBarHeight, width(), st::lineWidth);

	_scroll->setGeometry(0, topBarHeight, width(), height() - topBarHeight);
	if (_inner) {
		_inner->setViewportHeight(_scroll->height());
		_inner->resizeToWidth(_scroll->width());
	}
}

object_ptr<Window::SectionWidget> Memento::createWidget(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		Window::Column column,
		const QRect &geometry) {
	auto result = object_ptr<Widget>(parent, controller, _peer);
	result->setGeometry(geometry);
	return result;
}

} // namespace

void ShowDeletedMessages(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	controller->showSection(std::make_shared<Memento>(peer));
}

} // namespace Fenix
