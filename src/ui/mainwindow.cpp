#include "mainwindow.h"

#include <QAction>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QStatusBar>

#include "dialogs/configdialog.h"
#include "widgets/analyzermodulewidget.h"
#include "widgets/dashboardmodulewidget.h"
#include "widgets/databasemodulewidget.h"
#include "widgets/messagingmodulewidget.h"
#include "widgets/nodeemulatormodulewidget.h"
#include "widgets/packeteditormodulewidget.h"
#include "widgets/settingsmodulewidget.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_(std::make_unique<Ui::MainWindow>())
	, dashboardModuleWidget_(std::make_unique<DashboardModuleWidget>(this))
	, nodeEmulatorModuleWidget_(std::make_unique<NodeEmulatorModuleWidget>(this))
	, databaseModuleWidget_(std::make_unique<DatabaseModuleWidget>(this))
	, packetEditorModuleWidget_(std::make_unique<PacketEditorModuleWidget>(this))
	, messagingModuleWidget_(std::make_unique<MessagingModuleWidget>(this))
	, analyzerModuleWidget_(std::make_unique<AnalyzerModuleWidget>(this))
	, settingsModuleWidget_(std::make_unique<SettingsModuleWidget>(this))
	, config_(AppConfig::load())
{
	ui_->setupUi(this);
	setupCenterPages();
	setupToolbar();
	setupConnections();
	applyConfig(config_);
	if (!forgedPacketRepository_.load()) {
		appendApplicationLog(QStringLiteral("Failed to load packets from database: %1").arg(forgedPacketRepository_.lastError()));
	}
	dashboardModuleWidget_->setPackets(forgedPacketRepository_.packets());
	appendApplicationLog(QStringLiteral("Application started"));
	setMinimumSize(size());
}

MainWindow::~MainWindow() = default;

void MainWindow::applyConfig(const AppConfig &config)
{
	config_ = config;
	statusBar()->showMessage(QStringLiteral("Config: %1").arg(config_.configFilePath));
}

void MainWindow::setupToolbar()
{
	auto *editMenu = menuBar()->addMenu(QStringLiteral("Edit"));
	auto *configAction = editMenu->addAction(QStringLiteral("Configuration"));
	connect(configAction, &QAction::triggered, this, [this]() {
		showConfigDialog();
	});
	if (ui_->toolBarMain) {
		removeToolBar(ui_->toolBarMain);
		ui_->toolBarMain->hide();
	}
}

void MainWindow::setupConnections()
{
	connect(ui_->navList,
			&QListWidget::currentRowChanged,
			this,
			[this](int index) {
				switchCenterPage(index);
			}
	);

	dashboardModuleWidget_->setOnOpenPacketGenerator([this]() {
		ui_->navList->setCurrentRow(1);
		switchCenterPage(1);
		appendApplicationLog(QStringLiteral("Opened packet generator"));
	});
	dashboardModuleWidget_->setOnOpenPacket([this](const ForgedPacketRecord &packet) {
		openForgedPacket(packet);
	});
	dashboardModuleWidget_->setOnEditPacket([this](const QString &packetId) {
		editForgedPacket(packetId);
	});
	dashboardModuleWidget_->setOnRemovePacket([this](const QString &packetId) {
		removeForgedPacket(packetId);
	});
	dashboardModuleWidget_->setOnExportPacket([this](const ForgedPacketRecord &packet) {
		exportForgedPacket(packet);
	});
	dashboardModuleWidget_->setOnTransmitPacket([this](const ForgedPacketRecord &packet, const QString &target) {
		transmitForgedPacket(packet, target);
	});
	packetEditorModuleWidget_->setOnPacketForged([this](const ForgedPacketRecord &packet) {
		saveForgedPacket(packet);
	});
}

void MainWindow::showConfigDialog()
{
	auto *dialog = new ConfigDialog(config_, this);
	dialog->setAttribute(Qt::WA_DeleteOnClose, true);
	dialog->setOnApply([this](const AppConfig &config) {
		applyConfig(config);
	});
	dialog->setOnSave([this, dialog](const AppConfig &config) {
		applyConfig(config);
		if (config_.save()) {
			statusBar()->showMessage(QStringLiteral("Configuration saved"), 2000);
			appendApplicationLog(QStringLiteral("Configuration saved"));
		} else {
			statusBar()->showMessage(QStringLiteral("Failed to save configuration"), 2000);
			appendApplicationLog(QStringLiteral("Failed to save configuration"));
		}
		dialog->close();
	});
	dialog->show();
}

void MainWindow::setupCenterPages()
{
	ui_->stackMain->removeWidget(ui_->pageDashboard);
	ui_->stackMain->removeWidget(ui_->pagePacketGenerator);
	ui_->stackMain->removeWidget(ui_->pageNodeEmulator);
	ui_->stackMain->removeWidget(ui_->pageDatabase);
	ui_->stackMain->removeWidget(ui_->pageMessaging);
	ui_->stackMain->removeWidget(ui_->pageAnalyzer);
	ui_->stackMain->removeWidget(ui_->pageSettings);
	ui_->stackMain->insertWidget(0, dashboardModuleWidget_.get());
	ui_->stackMain->insertWidget(1, packetEditorModuleWidget_.get());
	ui_->stackMain->insertWidget(2, nodeEmulatorModuleWidget_.get());
	ui_->stackMain->insertWidget(3, databaseModuleWidget_.get());
	ui_->stackMain->insertWidget(4, messagingModuleWidget_.get());
	ui_->stackMain->insertWidget(5, analyzerModuleWidget_.get());
	ui_->stackMain->insertWidget(6, settingsModuleWidget_.get());
	ui_->navList->setCurrentRow(0);
	ui_->stackMain->setCurrentIndex(0);
}

void MainWindow::switchCenterPage(int index)
{
	if (index >= 0 && index < ui_->stackMain->count()) {
		ui_->stackMain->setCurrentIndex(index);
	}
}

void MainWindow::appendApplicationLog(const QString &message)
{
	if (ui_->logConsole) {
		ui_->logConsole->appendPlainText(message);
	}
}

void MainWindow::saveForgedPacket(const ForgedPacketRecord &packet)
{
	const bool exists = forgedPacketRepository_.contains(packet.id);
	if (!forgedPacketRepository_.upsertPacket(packet)) {
		appendApplicationLog(QStringLiteral("Failed to save packet to database: %1").arg(forgedPacketRepository_.lastError()));
		return;
	}
	if (exists) {
		dashboardModuleWidget_->updatePacket(packet);
		appendApplicationLog(QStringLiteral("Updated forged packet: %1").arg(packet.name));
	} else {
		dashboardModuleWidget_->addPacket(packet);
		appendApplicationLog(QStringLiteral("Added forged packet: %1").arg(packet.name));
	}
	packetEditorModuleWidget_->clearEditingPacket();
	ui_->navList->setCurrentRow(0);
	ui_->stackMain->setCurrentIndex(0);
}

void MainWindow::openForgedPacket(const ForgedPacketRecord &packet)
{
	packetEditorModuleWidget_->loadForgedPacket(packet);
	ui_->navList->setCurrentRow(1);
	ui_->stackMain->setCurrentIndex(1);
	appendApplicationLog(QStringLiteral("Loaded packet into generator: %1").arg(packet.name));
}

void MainWindow::editForgedPacket(const QString &packetId)
{
	const auto packet = forgedPacketRepository_.packetById(packetId);
	if (packet.id.isEmpty()) {
		appendApplicationLog(QStringLiteral("Edit failed: packet not found"));
		return;
	}
	openForgedPacket(packet);
}

void MainWindow::removeForgedPacket(const QString &packetId)
{
	if (!forgedPacketRepository_.removePacket(packetId)) {
		appendApplicationLog(QStringLiteral("Failed to remove packet from database: %1").arg(forgedPacketRepository_.lastError()));
		return;
	}
	dashboardModuleWidget_->removePacket(packetId);
	appendApplicationLog(QStringLiteral("Removed forged packet"));
}

void MainWindow::exportForgedPacket(const ForgedPacketRecord &packet)
{
	appendApplicationLog(QStringLiteral("Export completed: %1").arg(packet.name));
}

void MainWindow::transmitForgedPacket(const ForgedPacketRecord &packet, const QString &target)
{
	appendApplicationLog(QStringLiteral("Transmit requested for %1 via %2").arg(packet.name, target));
	statusBar()->showMessage(QStringLiteral("Transmit queued for %1 via %2").arg(packet.name, target), 3000);
}
