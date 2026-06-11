#include "tcapwidget.h"

#include "ui_TcapWidget.h"

TcapWidget::TcapWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::TcapWidget>())
{
	ui_->setupUi(this);
}

TcapWidget::~TcapWidget() = default;
