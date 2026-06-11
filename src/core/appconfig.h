#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

struct AppConfig
{
	QString windowTitle {QStringLiteral("Boar")};
	int windowWidth {800};
	int windowHeight {600};
	int payloadBoxMinHeight {160};
	int sendButtonWidth {120};
	QString lastPayload {};
	QString host {QStringLiteral("127.0.0.1")};
	int port {8080};
	QString theme {QStringLiteral("System")};
	QString configFilePath {defaultConfigFilePath()};

	static QString defaultConfigFilePath();
	QJsonObject toJson() const;
	static AppConfig fromJson(const QJsonObject &rootObject);
	static AppConfig load(const QString &filePath = defaultConfigFilePath());
	bool save(const QString &filePath = QString()) const;
};
