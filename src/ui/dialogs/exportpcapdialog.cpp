#include "exportpcapdialog.h"

#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QSpinBox>

#include "ui_ExportPcapDialog.h"

ExportPcapDialog::ExportPcapDialog(QWidget *parent)
	: QDialog(parent)
	, ui_(std::make_unique<Ui::ExportPcapDialog>())
{
	ui_->setupUi(this);
	ui_->spinVlanId->setVisible(false);
	ui_->labelVlanId->setVisible(false);
	connect(ui_->comboEtherType, &QComboBox::currentIndexChanged, this, [this](int index) {
		const bool showVlan = index == 1;
		ui_->spinVlanId->setVisible(showVlan);
		ui_->labelVlanId->setVisible(showVlan);
	});
	connect(ui_->comboTransportProtocol, &QComboBox::currentIndexChanged, this, [this](int index) {
		ui_->stackTransportProtocol->setCurrentIndex(index);
	});
	connect(ui_->btnBrowseDirectory, &QPushButton::clicked, this, [this]() {
		const QString directory = QFileDialog::getExistingDirectory(this, QStringLiteral("Select Export Directory"), ui_->editExportDirectory->text());
		if (!directory.isEmpty()) {
			ui_->editExportDirectory->setText(directory);
		}
	});
	connect(ui_->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
	connect(ui_->btnExport, &QPushButton::clicked, this, &QDialog::accept);
}

ExportPcapDialog::~ExportPcapDialog() = default;

QString ExportPcapDialog::exportDirectory() const
{
	return ui_->editExportDirectory->text();
}

QString ExportPcapDialog::sourceMac() const
{
	return ui_->editSourceMac ? ui_->editSourceMac->text().trimmed() : QString();
}

QString ExportPcapDialog::destinationMac() const
{
	return ui_->editDestinationMac ? ui_->editDestinationMac->text().trimmed() : QString();
}

QString ExportPcapDialog::sourceAddress() const
{
	return ui_->editSourceAddress ? ui_->editSourceAddress->text().trimmed() : QString();
}

QString ExportPcapDialog::destinationAddress() const
{
	return ui_->editDestinationAddress ? ui_->editDestinationAddress->text().trimmed() : QString();
}

quint16 ExportPcapDialog::sourcePort() const
{
	if (!ui_->comboTransportProtocol) {
		return 0;
	}
	if (ui_->comboTransportProtocol->currentIndex() == 0) {
		return static_cast<quint16>(ui_->spinUdpSrcPort ? ui_->spinUdpSrcPort->value() : 0);
	}
	if (ui_->comboTransportProtocol->currentIndex() == 1) {
		return static_cast<quint16>(ui_->spinTcpSrcPort ? ui_->spinTcpSrcPort->value() : 0);
	}
	return static_cast<quint16>(ui_->spinSctpSrcPort ? ui_->spinSctpSrcPort->value() : 0);
}

quint16 ExportPcapDialog::destinationPort() const
{
	if (!ui_->comboTransportProtocol) {
		return 0;
	}
	if (ui_->comboTransportProtocol->currentIndex() == 0) {
		return static_cast<quint16>(ui_->spinUdpDstPort ? ui_->spinUdpDstPort->value() : 0);
	}
	if (ui_->comboTransportProtocol->currentIndex() == 1) {
		return static_cast<quint16>(ui_->spinTcpDstPort ? ui_->spinTcpDstPort->value() : 0);
	}
	return static_cast<quint16>(ui_->spinSctpDstPort ? ui_->spinSctpDstPort->value() : 0);
}

quint32 ExportPcapDialog::sctpPayloadProtocolId() const
{
	return 3;
}

quint32 ExportPcapDialog::sctpVerificationTag() const
{
	return ui_->editVerificationTag ? ui_->editVerificationTag->text().toUInt() : 0;
}

int ExportPcapDialog::transportProtocolIndex() const
{
	return ui_->comboTransportProtocol ? ui_->comboTransportProtocol->currentIndex() : 0;
}

int ExportPcapDialog::ttl() const
{
	return ui_->spinTtl ? ui_->spinTtl->value() : 64;
}

bool ExportPcapDialog::isSctpSelected() const
{
	return transportProtocolIndex() == 2;
}
