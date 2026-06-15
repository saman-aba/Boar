#include "dashboardmodulewidget.h"

#include "ui_DashboardModuleWidget.h"

DashboardModuleWidget::DashboardModuleWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::DashboardModuleWidget>())
{
	ui_->setupUi(this);
}

DashboardModuleWidget::~DashboardModuleWidget() = default;
