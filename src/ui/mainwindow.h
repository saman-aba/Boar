#pragma once

#include <QMainWindow>

#include <memory>

#include "../core/appconfig.h"

class AnalyzerModuleWidget;
class DashboardModuleWidget;
class DatabaseModuleWidget;
class MessagingModuleWidget;
class PacketEditorModuleWidget;
class NodeEmulatorModuleWidget;
class SettingsModuleWidget;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private:
	void applyConfig(const AppConfig &config);
	void setupToolbar();
	void setupConnections();
	void showConfigDialog();
	void setupCenterPages();
	void switchCenterPage(int index);

	std::unique_ptr<Ui::MainWindow> ui_;
	std::unique_ptr<DashboardModuleWidget> dashboardModuleWidget_;
	std::unique_ptr<NodeEmulatorModuleWidget> nodeEmulatorModuleWidget_;
	std::unique_ptr<DatabaseModuleWidget> databaseModuleWidget_;
	std::unique_ptr<PacketEditorModuleWidget> packetEditorModuleWidget_;
	std::unique_ptr<MessagingModuleWidget> messagingModuleWidget_;
	std::unique_ptr<AnalyzerModuleWidget> analyzerModuleWidget_;
	std::unique_ptr<SettingsModuleWidget> settingsModuleWidget_;
	AppConfig config_ {};
};
