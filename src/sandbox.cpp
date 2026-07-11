
#include "sandbox.h"
#include "launcher.h"
#include "ui/mainwindow.h"
#include "ui/apptheme.h"

namespace Core {

Sandbox::Sandbox(int &argc, char **argv)
: QApplication(argc, argv){

}

int Sandbox::start() {
	setStyleSheet(appThemeStyleSheet());
	MainWindow mainWindow;
	mainWindow.show();

	_started = true;
	return exec();
}

Sandbox::~Sandbox() {
}


};
