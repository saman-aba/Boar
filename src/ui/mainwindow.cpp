#include "mainwindow.h"

#include <QAction>
#include <QMenuBar>
#include <QPushButton>
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

	if (auto *button = dashboardModuleWidget_->findChild<QPushButton *>(QStringLiteral("btnOpenPacketGen"))) {
		connect(button, &QPushButton::clicked,
				this,
				[this]() {
					ui_->navList->setCurrentRow(1);
					switchCenterPage(1);
				}
		);
	}
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
		} else {
			statusBar()->showMessage(QStringLiteral("Failed to save configuration"), 2000);
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
