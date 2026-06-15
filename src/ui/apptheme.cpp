#include "apptheme.h"

#include <QFile>
#include <QString>
#include <QStringList>

QString appThemeStyleSheet()
{
	const QStringList themeFiles = {
		QStringLiteral(":/ui/theme_base.qss"),
		QStringLiteral(":/ui/theme_widgets.qss"),
		QStringLiteral(":/ui/theme_module_shared.qss"),
		QStringLiteral(":/ui/theme_packet_generator.qss")
	};
	QString styleSheet;
	for (const QString &themeFile : themeFiles) {
		QFile styleFile(themeFile);
		if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			continue;
		}
		styleSheet += QString::fromUtf8(styleFile.readAll());
		styleSheet += QChar::fromLatin1('\n');
	}
	return styleSheet;
}
