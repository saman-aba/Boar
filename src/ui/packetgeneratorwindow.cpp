#include "packetgeneratorwindow.h"

#include <QComboBox>

#include "diametereditorwidget.h"
#include "tcapeditorwidget.h"
#include "ui_PacketGeneratorWindow.h"

PacketGeneratorWindow::PacketGeneratorWindow(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::PacketGeneratorWindow>())
{
	ui_->setupUi(this);
	auto *diameterWidget = new DiameterEditorWidget(this);
	auto *tcapWidget = new TcapEditorWidget(this);
	ui_->protocolStack->addWidget(diameterWidget);
	ui_->protocolStack->addWidget(new QWidget(this));
	ui_->protocolStack->addWidget(tcapWidget);
	connect(ui_->comboProtocolSelector, &QComboBox::currentIndexChanged, this, [this](int index) {
		ui_->protocolStack->setCurrentIndex(index);
	});
}

PacketGeneratorWindow::~PacketGeneratorWindow() = default;
