#include "analyzermodulewidget.h"

#include "ui_AnalyzerModuleWidget.h"

AnalyzerModuleWidget::AnalyzerModuleWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::AnalyzerModuleWidget>())
{
	ui_->setupUi(this);
}

AnalyzerModuleWidget::~AnalyzerModuleWidget() = default;
