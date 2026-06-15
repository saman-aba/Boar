#include "databasemodulewidget.h"

#include "ui_DatabaseModuleWidget.h"

DatabaseModuleWidget::DatabaseModuleWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::DatabaseModuleWidget>())
{
	ui_->setupUi(this);
}

DatabaseModuleWidget::~DatabaseModuleWidget() = default;
