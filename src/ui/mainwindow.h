#pragma once

#include <QMainWindow>

#include "../core/appconfig.h"
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow
{
public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override = default;

private:
	void applyConfig(const AppConfig &config);
	void setupToolbar();
	void setupConnections();
	void showConfigDialog();

	Ui::MainWindow ui_;
	AppConfig config_ {};
};
