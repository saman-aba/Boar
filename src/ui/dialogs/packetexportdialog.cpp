#include "packetexportdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPlainTextEdit>
#include <QVBoxLayout>

PacketExportDialog::PacketExportDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(QStringLiteral("Export Packet"));
	auto *layout = new QVBoxLayout(this);
	auto *form = new QFormLayout();
	exportTypeCombo_ = new QComboBox(this);
	exportTypeCombo_->addItems({QStringLiteral("PCAP"), QStringLiteral("HEX DUMP"), QStringLiteral("HEX STRING"), QStringLiteral("Binary" )});
	descriptionEdit_ = new QPlainTextEdit(this);
	descriptionEdit_->setReadOnly(true);
	descriptionEdit_->setMaximumHeight(60);
	descriptionEdit_->setPlainText(QStringLiteral("Select export format, then use save file dialog in dashboard action."));
	form->addRow(QStringLiteral("Export format"), exportTypeCombo_);
	form->addRow(QStringLiteral("Info"), descriptionEdit_);
	buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addLayout(form);
	layout->addWidget(buttonBox_);
	connect(buttonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

PacketExportDialog::~PacketExportDialog() = default;

PacketExportDialog::ExportType PacketExportDialog::exportType() const
{
	switch (exportTypeCombo_->currentIndex()) {
	case 0:
		return ExportType::Pcap;
	case 1:
		return ExportType::HexDump;
	case 2:
		return ExportType::HexString;
	default:
		return ExportType::Binary;
	}
}
