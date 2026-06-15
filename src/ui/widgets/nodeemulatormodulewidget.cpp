#include "nodeemulatormodulewidget.h"

#include "ui_NodeEmulatorModuleWidget.h"

NodeEmulatorModuleWidget::NodeEmulatorModuleWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::NodeEmulatorModuleWidget>())
{
	ui_->setupUi(this);
}

NodeEmulatorModuleWidget::~NodeEmulatorModuleWidget() = default;
