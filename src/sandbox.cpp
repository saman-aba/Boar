
#include "sandbox.h"
#include "launcher.h"
#include "ui/mainwindow.h"
#include "ui/apptheme.h"

#include <QDir>
#include <QIcon>
#include <QPixmap>
#include <QSplashScreen>
#include <QStandardPaths>

namespace Core {

Sandbox::Sandbox(int &argc, char **argv)
: QApplication(argc, argv){
	setApplicationName(QStringLiteral("Boar"));
	setApplicationDisplayName(QStringLiteral("Boar"));
	setWindowIcon(QIcon(QStringLiteral(":/branding/app_icon.png")));
}

int Sandbox::start() {
	const QString applicationDataPath =
			QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	if (!applicationDataPath.isEmpty()) {
		QDir().mkpath(applicationDataPath);
	}

	QPixmap splashPixmap(QStringLiteral(":/branding/app_icon.png"));
	splashPixmap = splashPixmap.scaled(640,
			480,
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation);
	QSplashScreen splash(splashPixmap);
	splash.show();
	splash.showMessage(QStringLiteral("Starting Boar..."),
			Qt::AlignBottom | Qt::AlignHCenter,
			Qt::white);
	processEvents();

	setStyleSheet(appThemeStyleSheet());
	MainWindow mainWindow;
	mainWindow.show();
	splash.finish(&mainWindow);

	_started = true;
	return exec();
}

Sandbox::~Sandbox() {
}


};
