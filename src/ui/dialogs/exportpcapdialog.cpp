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
