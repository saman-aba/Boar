#include "mainwindow.h"

#include <QAction>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>

#include "dialogs/configdialog.h"
#include "dialogs/newpacketdialog.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, config_(AppConfig::load())
{
	ui_.setupUi(this);
	setupToolbar();
	setupConnections();
	applyConfig(config_);
}

void MainWindow::applyConfig(const AppConfig &config)
{
	config_ = config;
	setWindowTitle(config_.windowTitle);
	resize(config_.windowWidth, config_.windowHeight);
	ui_.payloadTextEdit->setMinimumHeight(config_.payloadBoxMinHeight);
	ui_.sendButton->setFixedWidth(config_.sendButtonWidth);
	ui_.payloadTextEdit->setPlainText(config_.lastPayload);
	statusBar()->showMessage(QStringLiteral("Config: %1").arg(config_.configFilePath));
}

void MainWindow::setupToolbar()
{
	auto *editMenu = menuBar()->addMenu(QStringLiteral("Edit"));
	auto *configAction = editMenu->addAction(QStringLiteral("Configuration"));
	connect(configAction, &QAction::triggered, this, [this]() {
		showConfigDialog();
	});
	removeToolBar(ui_.mainToolBar);
	ui_.mainToolBar->hide();
}

void MainWindow::setupConnections()
{
	connect(ui_.sendButton, &QPushButton::clicked, this, [this]() {
		config_.lastPayload = ui_.payloadTextEdit->toPlainText();
		statusBar()->showMessage(QStringLiteral("Payload captured"), 2000);
	});
	connect(ui_.newPacketButton, &QPushButton::clicked, this, [this]() {
		auto *dialog = new NewPacketDialog(this);
		dialog->setAttribute(Qt::WA_DeleteOnClose, true);
		dialog->show();
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
