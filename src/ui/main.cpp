#include <QApplication>

#include "apptheme.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication application(argc, argv);
	application.setStyleSheet(appThemeStyleSheet());
	MainWindow mainWindow;
	mainWindow.resize(800, 600);
	mainWindow.show();
	return application.exec();
}
