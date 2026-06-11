#include <QApplication>

#include "boar_mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication application(argc, argv);
	BoarMainWindow mainWindow;
	mainWindow.show();
	return application.exec();
}
