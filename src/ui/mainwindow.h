#pragma once

#include <QMainWindow>

#include <memory>

#include "../core/appconfig.h"

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

	std::unique_ptr<Ui::MainWindow> ui_;
	AppConfig config_ {};
};
