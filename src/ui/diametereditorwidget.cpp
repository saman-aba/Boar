#include "diametereditorwidget.h"

#include "ui_DiameterEditorWidget.h"

DiameterEditorWidget::DiameterEditorWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::DiameterEditorWidget>())
{
	ui_->setupUi(this);
}

DiameterEditorWidget::~DiameterEditorWidget() = default;
