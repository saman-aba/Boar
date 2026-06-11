#include "diameterwidget.h"

#include <QDialog>
#include <QPushButton>

#include "addavpdialog.h"
#include "ui_DiameterWidget.h"

DiameterWidget::DiameterWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::DiameterWidget>())
{
	ui_->setupUi(this);
	connect(ui_->addAvpButton, &QPushButton::clicked, this, [this]() {
		AddAvpDialog dialog(this);
		if (dialog.exec() == QDialog::Accepted) {
			ui_->avpListWidget->addItem(dialog.avpDisplayText());
		}
	});
}

DiameterWidget::~DiameterWidget() = default;
