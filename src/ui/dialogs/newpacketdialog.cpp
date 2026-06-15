#include "newpacketdialog.h"

#include <QComboBox>
#include <QPushButton>

#include "../widgets/diameterwidget.h"
#include "../widgets/gtpwidget.h"
#include "../widgets/tcapwidget.h"
#include "ui_NewPacketDialog.h"

NewPacketDialog::NewPacketDialog(QWidget *parent)
	: QDialog(parent)
	, ui_(std::make_unique<Ui::NewPacketDialog>())
{
	ui_->setupUi(this);
	ui_->protocolComboBox->addItems(
		{
			QStringLiteral("Diameter"),
			QStringLiteral("TCAP"),
			QStringLiteral("GTP")
		}
	);
	auto *diameterWidget = new DiameterWidget(this);
	auto *tcapWidget = new TcapWidget(this);
	auto *gtpWidget = new GtpWidget(this);
	ui_->protocolStackedWidget->addWidget(diameterWidget);
	ui_->protocolStackedWidget->addWidget(tcapWidget);
	ui_->protocolStackedWidget->addWidget(gtpWidget);
	connect(ui_->protocolComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
		ui_->protocolStackedWidget->setCurrentIndex(index);
	});
	connect(ui_->closeButton, &QPushButton::clicked, this, &QDialog::close);
}

NewPacketDialog::~NewPacketDialog() = default;
