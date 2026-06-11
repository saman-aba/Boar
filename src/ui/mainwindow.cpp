#include "mainwindow.h"

#include <QAction>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>

#include "dialogs/configdialog.h"
#include "packetgeneratorwindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_(std::make_unique<Ui::MainWindow>())
	, config_(AppConfig::load())
{
	ui_->setupUi(this);
	setupToolbar();
	setupConnections();
	applyConfig(config_);
}

MainWindow::~MainWindow() = default;

void MainWindow::applyConfig(const AppConfig &config)
{
	config_ = config;
	setWindowTitle(config_.windowTitle);
	resize(config_.windowWidth, config_.windowHeight);
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
	connect(ui_->btnOpenPacketGen, &QPushButton::clicked, this, []() {
		auto *window = new PacketGeneratorWindow();
		window->setAttribute(Qt::WA_DeleteOnClose, true);
		window->show();
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
		} else {
			statusBar()->showMessage(QStringLiteral("Failed to save configuration"), 2000);
		}
		dialog->close();
	});
	dialog->show();
}
