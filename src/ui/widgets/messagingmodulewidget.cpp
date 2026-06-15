#include "messagingmodulewidget.h"

#include "ui_MessagingModuleWidget.h"

MessagingModuleWidget::MessagingModuleWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::MessagingModuleWidget>())
{
	ui_->setupUi(this);
}

MessagingModuleWidget::~MessagingModuleWidget() = default;
