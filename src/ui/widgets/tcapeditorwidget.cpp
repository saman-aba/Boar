#include "tcapeditorwidget.h"

#include "ui_TcapEditorWidget.h"

TcapEditorWidget::TcapEditorWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::TcapEditorWidget>())
{
	ui_->setupUi(this);
}

TcapEditorWidget::~TcapEditorWidget() = default;
