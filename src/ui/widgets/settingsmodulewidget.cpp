#include "settingsmodulewidget.h"

#include "ui_SettingsModuleWidget.h"

SettingsModuleWidget::SettingsModuleWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::SettingsModuleWidget>())
{
	ui_->setupUi(this);
}

SettingsModuleWidget::~SettingsModuleWidget() = default;
