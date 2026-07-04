#include <QApplication>
#include <QDir>
#include <QStandardPaths>

extern "C" {
#include "../core/packet-builder.h"
}
#include "apptheme.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication application(argc, argv);
	application.setStyleSheet(appThemeStyleSheet());
	MainWindow mainWindow;
	mainWindow.resize(800, 600);
	mainWindow.show();

	const QString basePath = QStandardPaths::writableLocation(
				QStandardPaths::AppDataLocation);
	QString dbPath = QDir(basePath).filePath(QStringLiteral("boar.sqlite3"));

	QByteArray dbPathUtf8 = dbPath.toUtf8();
	packet_builder_init(dbPathUtf8.constData());
	return application.exec();
}
