#pragma once

#include "mainwindow.h"

class BoarMainWindow : public MainWindow
{
public:
	explicit BoarMainWindow(QWidget *parent = nullptr)
		: MainWindow(parent)
	{
		resize(800, 600);
	}
	~BoarMainWindow() override = default;
};
