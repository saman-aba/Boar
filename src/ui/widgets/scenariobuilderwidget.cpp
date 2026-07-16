#include "scenariobuilderwidget.h"

#include "ui_SenarioBuilder.h"

ScenarioBuilderWidget::ScenarioBuilderWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::ScenarioBuilderWidget>())
{
	ui_->setupUi(this);
}

ScenarioBuilderWidget::~ScenarioBuilderWidget() = default;
