#include "addtcapparameterdialog.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>

#include "ui_AddTcapParameterDialog.h"

AddTcapParameterDialog::AddTcapParameterDialog(QWidget *parent)
	: QDialog(parent)
	, ui_(std::make_unique<Ui::AddTcapParameterDialog>())
{
	ui_->setupUi(this);
	connect(ui_->comboParameterName, &QComboBox::currentIndexChanged, this, [this](int) { refreshTypeName(); });
	connect(ui_->buttonAdd, &QPushButton::clicked, this, &QDialog::accept);
	connect(ui_->buttonCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddTcapParameterDialog::~AddTcapParameterDialog() = default;

void AddTcapParameterDialog::setParameterOptions(const QStringList &options)
{
	setParameterDetails(options, {});
}

void AddTcapParameterDialog::setParameterDetails(const QStringList &options, const QHash<QString, QString> &typeNames)
{
	if (!ui_->comboParameterName) {
		return;
	}
	typeNames_ = typeNames;
	ui_->comboParameterName->clear();
	ui_->comboParameterName->addItems(options);
	refreshTypeName();
}

QString AddTcapParameterDialog::selectedParameter() const
{
	return ui_->comboParameterName ? ui_->comboParameterName->currentText() : QString();
}

QString AddTcapParameterDialog::selectedTypeName() const
{
	return ui_->editTypeName ? ui_->editTypeName->text().trimmed() : QString();
}

QString AddTcapParameterDialog::parameterValue() const
{
	return ui_->editParameterValue ? ui_->editParameterValue->text().trimmed() : QString();
}

void AddTcapParameterDialog::refreshTypeName()
{
	if (!ui_->editTypeName) {
		return;
	}
	ui_->editTypeName->setText(typeNames_.value(selectedParameter()));
}
