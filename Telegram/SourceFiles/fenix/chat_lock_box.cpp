/*
This file is part of Novagram Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/chat_lock_box.h"

#include "fenix/chat_lock.h"
#include "lang/lang_keys.h"
#include "ui/layers/generic_box.h"
#include "ui/rp_widget.h"
#include "ui/ui_utility.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/fields/password_input.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"

#include <memory>

namespace Fenix {
namespace {

// PasswordInput is a MaskedInputField (RpWidgetBase<QLineEdit>), not an RpWidget,
// so GenericBox::addRow won't take it directly. Wrap it in a plain RpWidget row
// that tracks the field's size, mirroring settings_local_passcode.
[[nodiscard]] not_null<Ui::PasswordInput*> AddPasswordRow(
		not_null<Ui::GenericBox*> box,
		rpl::producer<QString> placeholder) {
	auto container = object_ptr<Ui::RpWidget>(box);
	const auto raw = container.data();
	const auto field = Ui::CreateChild<Ui::PasswordInput>(
		raw,
		st::defaultInputField,
		std::move(placeholder));
	raw->resize(raw->width(), field->height());
	raw->widthValue() | rpl::on_next([=](int width) {
		field->resize(width, field->height());
		field->moveToLeft(0, 0);
	}, raw->lifetime());
	field->heightValue() | rpl::on_next([=](int height) {
		raw->resize(raw->width(), height);
	}, raw->lifetime());
	box->addRow(std::move(container));
	return field;
}

void FillPasscodeBox(
		not_null<Ui::GenericBox*> box,
		PeerId peerId,
		rpl::producer<QString> submitText,
		Fn<void()> onUnlocked) {
	box->setTitle(tr::lng_fenix_chat_lock_title());

	const auto field = AddPasswordRow(box, tr::lng_fenix_chat_lock_enter());
	box->setFocusCallback([=] { field->setFocusFast(); });

	const auto submit = [=] {
		const auto input = field->getLastText();
		if (input.isEmpty()) {
			field->showError();
			return;
		}
		if (!GetChatLock().check(peerId, input)) {
			field->showError();
			field->setText(QString());
			return;
		}
		GetChatLock().markUnlocked(peerId);
		if (onUnlocked) {
			onUnlocked();
		}
		box->closeBox();
	};
	QObject::connect(
		field,
		&Ui::MaskedInputField::submitted,
		field,
		submit);

	box->addButton(std::move(submitText), submit);
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

} // namespace

void ChatLockUnlockBox(
		not_null<Ui::GenericBox*> box,
		PeerId peerId,
		Fn<void()> onUnlocked) {
	FillPasscodeBox(
		box,
		peerId,
		tr::lng_fenix_chat_lock_open(),
		std::move(onUnlocked));
}

void ChatLockVerifyBox(
		not_null<Ui::GenericBox*> box,
		PeerId peerId,
		Fn<void()> onVerified) {
	FillPasscodeBox(
		box,
		peerId,
		tr::lng_fenix_chat_lock_continue(),
		std::move(onVerified));
}

void ChatLockChoiceBox(
		not_null<Ui::GenericBox*> box,
		Fn<void(bool individual)> onChoose) {
	box->setTitle(tr::lng_fenix_chat_lock_how());

	// 0 = common (shared) passcode, 1 = separate (per-chat) passcode.
	const auto group = std::make_shared<Ui::RadiobuttonGroup>(0);
	box->addRow(object_ptr<Ui::Radiobutton>(
		box,
		group,
		0,
		tr::lng_fenix_chat_lock_common(tr::now)));
	box->addRow(object_ptr<Ui::Radiobutton>(
		box,
		group,
		1,
		tr::lng_fenix_chat_lock_separate(tr::now)));

	box->addButton(tr::lng_fenix_chat_lock_continue(), [=] {
		const auto individual = (group->current() == 1);
		box->closeBox();
		if (onChoose) {
			onChoose(individual);
		}
	});
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

void ChatLockCreateBox(
		not_null<Ui::GenericBox*> box,
		PeerId peerId,
		bool individual,
		Fn<void()> onDone) {
	box->setTitle(tr::lng_fenix_chat_lock_set());

	const auto enter = AddPasswordRow(box, tr::lng_fenix_chat_lock_enter());
	const auto confirm = AddPasswordRow(box, tr::lng_fenix_chat_lock_confirm());
	box->setFocusCallback([=] { enter->setFocusFast(); });

	const auto save = [=] {
		const auto first = enter->getLastText();
		const auto second = confirm->getLastText();
		if (first.isEmpty()) {
			enter->showError();
			return;
		}
		if (first != second) {
			confirm->showError();
			confirm->setText(QString());
			return;
		}
		auto &lock = GetChatLock();
		if (individual) {
			lock.addIndividualLock(peerId, first);
		} else {
			lock.changeCommonPassword(first);
			lock.addCommonLock(peerId);
		}
		// Don't gate the chat the user is locking from right now.
		lock.markUnlocked(peerId);
		if (onDone) {
			onDone();
		}
		box->closeBox();
	};
	QObject::connect(
		enter,
		&Ui::MaskedInputField::submitted,
		enter,
		[=](Qt::KeyboardModifiers) { confirm->setFocusFast(); });
	QObject::connect(
		confirm,
		&Ui::MaskedInputField::submitted,
		confirm,
		save);

	box->addButton(tr::lng_settings_save(), save);
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

void ChatLockCommonPasswordBox(
		not_null<Ui::GenericBox*> box,
		Fn<void()> onDone) {
	box->setTitle(tr::lng_fenix_chat_lock_change_common());

	// The common passcode opens every chat on it, so a new one needs the
	// current one first: else anyone at the computer could set their own
	// from the settings and open those chats with it.
	const auto changing = GetChatLock().hasCommonPassword();
	const auto current = changing
		? AddPasswordRow(box, tr::lng_passcode_enter_old()).get()
		: nullptr;
	const auto enter = AddPasswordRow(box, changing
		? tr::lng_passcode_enter_new()
		: tr::lng_fenix_chat_lock_enter());
	const auto confirm = AddPasswordRow(box, changing
		? tr::lng_passcode_confirm_new()
		: tr::lng_fenix_chat_lock_confirm());
	box->setFocusCallback([=] {
		if (current) {
			current->setFocusFast();
		} else {
			enter->setFocusFast();
		}
	});

	const auto save = [=] {
		if (current && !GetChatLock().checkCommon(current->getLastText())) {
			current->showError();
			current->setText(QString());
			current->setFocusFast();
			return;
		}
		const auto first = enter->getLastText();
		const auto second = confirm->getLastText();
		if (first.isEmpty()) {
			enter->showError();
			return;
		}
		if (first != second) {
			confirm->showError();
			confirm->setText(QString());
			return;
		}
		// Overwrites the shared passcode; chats on it read it live, so nothing
		// per-chat needs re-syncing.
		GetChatLock().changeCommonPassword(first);
		if (onDone) {
			onDone();
		}
		box->closeBox();
	};
	if (current) {
		QObject::connect(
			current,
			&Ui::MaskedInputField::submitted,
			current,
			[=](Qt::KeyboardModifiers) { enter->setFocusFast(); });
	}
	QObject::connect(
		enter,
		&Ui::MaskedInputField::submitted,
		enter,
		[=](Qt::KeyboardModifiers) { confirm->setFocusFast(); });
	QObject::connect(
		confirm,
		&Ui::MaskedInputField::submitted,
		confirm,
		save);

	box->addButton(tr::lng_settings_save(), save);
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

} // namespace Fenix
