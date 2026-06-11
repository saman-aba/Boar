#include "gtpwidget.h"

#include "ui_GtpWidget.h"

GtpWidget::GtpWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::GtpWidget>())
{
	ui_->setupUi(this);
}

GtpWidget::~GtpWidget() = default;
