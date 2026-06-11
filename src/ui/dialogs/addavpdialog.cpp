#include "addavpdialog.h"

#include <QPushButton>

#include "ui_AddAvpDialog.h"

AddAvpDialog::AddAvpDialog(QWidget *parent)
	: QDialog(parent)
	, ui_(std::make_unique<Ui::AddAvpDialog>())
{
	ui_->setupUi(this);
	connect(ui_->addButton, &QPushButton::clicked, this, &QDialog::accept);
	connect(ui_->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

AddAvpDialog::~AddAvpDialog() = default;

QString AddAvpDialog::avpDisplayText() const
{
	return QStringLiteral("%1 = %2").arg(ui_->avpNameLineEdit->text(), ui_->avpValueLineEdit->text());
}
