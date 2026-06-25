/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.
*/
#include "fenix/demo_loading_box.h"

#include "lang/lang_keys.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "ui/effects/radial_animation.h"
#include "ui/layers/generic_box.h"
#include "ui/painter.h"
#include "ui/rp_widget.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/ui_utility.h"
#include "boxes/abstract_box.h"

namespace Fenix {
namespace {

constexpr auto kSpinnerSize = 56;
constexpr auto kSpinnerStrokeWidth = 4;

class SpinnerWidget final : public Ui::RpWidget {
public:
	explicit SpinnerWidget(QWidget *parent)
	: Ui::RpWidget(parent)
	, _animation([=] { update(); }, st::defaultInfiniteRadialAnimation) {
		resize(kSpinnerSize, kSpinnerSize);
		_animation.start();
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		const auto state = _animation.computeState();
		Ui::InfiniteRadialAnimation::Draw(
			p,
			state,
			QPoint(0, 0),
			QSize(kSpinnerSize, kSpinnerSize),
			width(),
			QPen(st::windowBgActive),
			kSpinnerStrokeWidth);
	}

private:
	Ui::InfiniteRadialAnimation _animation;
};

void DemoLoadingBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(rpl::single(QString(u"Demo Account"_q)));
	box->setNoContentMargin(false);

	const auto content = box->verticalLayout();

	content->add(
		object_ptr<SpinnerWidget>(content),
		st::boxRowPadding + style::margins(0, 24, 0, 16),
		style::al_top);

	const auto label = content->add(
		object_ptr<Ui::FlatLabel>(
			content,
			QString(u"This account is a Demo account for "
				"testing the application."_q),
			st::boxLabel),
		st::boxRowPadding + style::margins(0, 0, 0, 16));
	label->setTextColorOverride(st::windowSubTextFg->c);
	label->resizeToWidth(st::boxWidth - st::boxRowPadding.left()
		- st::boxRowPadding.right());
	label->setBreakEverywhere(true);
	label->setSelectable(false);
}

} // namespace

QPointer<Ui::BoxContent> ShowDemoLoadingBox() {
	auto box = Box(DemoLoadingBox);
	const auto raw = box.data();
	Ui::show(std::move(box), Ui::LayerOption::KeepOther);
	return QPointer<Ui::BoxContent>(raw);
}

} // namespace Fenix
