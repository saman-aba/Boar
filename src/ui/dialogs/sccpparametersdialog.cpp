#include "sccpparametersdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

SccpParametersDialog::SccpParametersDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(QStringLiteral("SCCP / M3UA Parameters"));
	resize(460, 0);
	auto *layout = new QVBoxLayout(this);
	auto *formLayout = new QFormLayout();
	packetNameEdit_ = new QLineEdit(this);
	sourceMacEdit_ = new QLineEdit(this);
	destinationMacEdit_ = new QLineEdit(this);
	sourceAddressEdit_ = new QLineEdit(this);
	destinationAddressEdit_ = new QLineEdit(this);
	sourcePortSpin_ = new QSpinBox(this);
	destinationPortSpin_ = new QSpinBox(this);
	verificationTagEdit_ = new QLineEdit(this);
	payloadProtocolIdEdit_ = new QLineEdit(this);
	ttlSpin_ = new QSpinBox(this);
	sctpChunkCombo_ = new QComboBox(this);
	sccpMessageCombo_ = new QComboBox(this);
	includeM3uaCheck_ = new QCheckBox(this);
	buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	packetNameEdit_->setText(QStringLiteral("forged_sccp_packet"));
	sourceMacEdit_->setText(QStringLiteral("00:11:22:33:44:55"));
	destinationMacEdit_->setText(QStringLiteral("66:77:88:99:AA:BB"));
	sourceAddressEdit_->setText(QStringLiteral("10.0.0.1"));
	destinationAddressEdit_->setText(QStringLiteral("10.0.0.2"));
	sourcePortSpin_->setRange(1, 65535);
	destinationPortSpin_->setRange(1, 65535);
	sourcePortSpin_->setValue(2905);
	destinationPortSpin_->setValue(2905);
	verificationTagEdit_->setText(QStringLiteral("0"));
	payloadProtocolIdEdit_->setText(QStringLiteral("3"));
	ttlSpin_->setRange(1, 255);
	ttlSpin_->setValue(64);
	sctpChunkCombo_->addItems({QStringLiteral("DATA"), QStringLiteral("I-DATA")});
	sccpMessageCombo_->addItems({QStringLiteral("UDT"), QStringLiteral("XUDT"), QStringLiteral("LUDT")});
	includeM3uaCheck_->setChecked(true);
	includeM3uaCheck_->setText(QStringLiteral("Wrap SCCP inside M3UA DATA"));
	formLayout->addRow(QStringLiteral("Packet Name"), packetNameEdit_);
	formLayout->addRow(QStringLiteral("Source MAC"), sourceMacEdit_);
	formLayout->addRow(QStringLiteral("Destination MAC"), destinationMacEdit_);
	formLayout->addRow(QStringLiteral("Source IP"), sourceAddressEdit_);
	formLayout->addRow(QStringLiteral("Destination IP"), destinationAddressEdit_);
	formLayout->addRow(QStringLiteral("Source SCTP Port"), sourcePortSpin_);
	formLayout->addRow(QStringLiteral("Destination SCTP Port"), destinationPortSpin_);
	formLayout->addRow(QStringLiteral("Verification Tag"), verificationTagEdit_);
	formLayout->addRow(QStringLiteral("Payload Protocol ID"), payloadProtocolIdEdit_);
	formLayout->addRow(QStringLiteral("TTL"), ttlSpin_);
	formLayout->addRow(QStringLiteral("SCTP Chunk Type"), sctpChunkCombo_);
	formLayout->addRow(QStringLiteral("SCCP Message Type"), sccpMessageCombo_);
	formLayout->addRow(QString(), includeM3uaCheck_);
	layout->addLayout(formLayout);
	layout->addWidget(buttonBox_);
	connect(buttonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SccpParametersDialog::~SccpParametersDialog() = default;

QString SccpParametersDialog::sourceMac() const
{
	return sourceMacEdit_->text().trimmed();
}

QString SccpParametersDialog::destinationMac() const
{
	return destinationMacEdit_->text().trimmed();
}

QString SccpParametersDialog::sourceAddress() const
{
	return sourceAddressEdit_->text().trimmed();
}

QString SccpParametersDialog::destinationAddress() const
{
	return destinationAddressEdit_->text().trimmed();
}

quint16 SccpParametersDialog::sourcePort() const
{
	return static_cast<quint16>(sourcePortSpin_->value());
}

quint16 SccpParametersDialog::destinationPort() const
{
	return static_cast<quint16>(destinationPortSpin_->value());
}

quint32 SccpParametersDialog::verificationTag() const
{
	return verificationTagEdit_->text().toUInt();
}

quint32 SccpParametersDialog::payloadProtocolId() const
{
	return payloadProtocolIdEdit_->text().toUInt();
}

int SccpParametersDialog::ttl() const
{
	return ttlSpin_->value();
}

QString SccpParametersDialog::sctpChunkType() const
{
	return sctpChunkCombo_->currentText();
}

QString SccpParametersDialog::sccpMessageType() const
{
	return sccpMessageCombo_->currentText();
}

bool SccpParametersDialog::includeM3ua() const
{
	return includeM3uaCheck_->isChecked();
}

QString SccpParametersDialog::packetName() const
{
	return packetNameEdit_->text().trimmed();
}
