#include "addtcapparameterdialog.h"

#include <QComboBox>
#include <QPushButton>
#include <QStringList>

#include "ui_AddTcapParameterDialog.h"

AddTcapParameterDialog::AddTcapParameterDialog(QWidget *parent)
	: QDialog(parent)
	, ui_(std::make_unique<Ui::AddTcapParameterDialog>())
{
	ui_->setupUi(this);
	connect(ui_->buttonAdd, &QPushButton::clicked, this, &QDialog::accept);
	connect(ui_->buttonCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddTcapParameterDialog::~AddTcapParameterDialog() = default;

void AddTcapParameterDialog::setParameterOptions(const QStringList &options)
{
	if (!ui_->comboParameterName) {
		return;
	}
	ui_->comboParameterName->clear();
	ui_->comboParameterName->addItems(options);
}

QString AddTcapParameterDialog::selectedParameter() const
{
	return ui_->comboParameterName ? ui_->comboParameterName->currentText() : QString();
}

QString AddTcapParameterDialog::parameterValue() const
{
	return ui_->editParameterValue ? ui_->editParameterValue->text().trimmed() : QString();
}
