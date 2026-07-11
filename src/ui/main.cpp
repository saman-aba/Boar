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
	const QString basePath = QStandardPaths::writableLocation(
				QStandardPaths::AppDataLocation);
	QString dbPath = QDir(basePath).filePath(QStringLiteral("boar.sqlite3"));

	QByteArray dbPathUtf8 = dbPath.toUtf8();
	packet_builder_init(dbPathUtf8.constData());
}
