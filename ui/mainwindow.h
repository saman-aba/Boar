#pragma once

#include <QMainWindow>

#include "ui_MainWindow.h"

class MainWindow : public QMainWindow
{
public:
	explicit MainWindow(QWidget *parent = nullptr)
		: QMainWindow(parent)
	{
		ui_.setupUi(this);
		setWindowTitle(QStringLiteral("Boar"));
	}
	~MainWindow() override = default;

private:
	Ui::MainWindow ui_;
};
