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

	static QString defaultConfigFilePath()
	{
		return QDir::homePath() + QStringLiteral("/.config/boar/config.json");
	}

	QJsonObject toJson() const
	{
		QJsonObject uiObject;
		uiObject.insert(QStringLiteral("windowTitle"), windowTitle);
		uiObject.insert(QStringLiteral("windowWidth"), windowWidth);
		uiObject.insert(QStringLiteral("windowHeight"), windowHeight);
		uiObject.insert(QStringLiteral("payloadBoxMinHeight"), payloadBoxMinHeight);
		uiObject.insert(QStringLiteral("sendButtonWidth"), sendButtonWidth);

		QJsonObject networkObject;
		networkObject.insert(QStringLiteral("host"), host);
		networkObject.insert(QStringLiteral("port"), port);

		QJsonObject appearanceObject;
		appearanceObject.insert(QStringLiteral("theme"), theme);

		QJsonObject coreObject;
		coreObject.insert(QStringLiteral("lastPayload"), lastPayload);
		coreObject.insert(QStringLiteral("configFilePath"), configFilePath);

		QJsonObject rootObject;
		rootObject.insert(QStringLiteral("ui"), uiObject);
		rootObject.insert(QStringLiteral("network"), networkObject);
		rootObject.insert(QStringLiteral("appearance"), appearanceObject);
		rootObject.insert(QStringLiteral("core"), coreObject);
		return rootObject;
	}

	static AppConfig fromJson(const QJsonObject &rootObject)
	{
		AppConfig config;
		const QJsonObject uiObject = rootObject.value(QStringLiteral("ui")).toObject();
		const QJsonObject networkObject = rootObject.value(QStringLiteral("network")).toObject();
		const QJsonObject appearanceObject = rootObject.value(QStringLiteral("appearance")).toObject();
		const QJsonObject coreObject = rootObject.value(QStringLiteral("core")).toObject();

		config.windowTitle = uiObject.value(QStringLiteral("windowTitle")).toString(config.windowTitle);
		config.windowWidth = uiObject.value(QStringLiteral("windowWidth")).toInt(config.windowWidth);
		config.windowHeight = uiObject.value(QStringLiteral("windowHeight")).toInt(config.windowHeight);
		config.payloadBoxMinHeight = uiObject.value(QStringLiteral("payloadBoxMinHeight")).toInt(config.payloadBoxMinHeight);
		config.sendButtonWidth = uiObject.value(QStringLiteral("sendButtonWidth")).toInt(config.sendButtonWidth);
		config.host = networkObject.value(QStringLiteral("host")).toString(config.host);
		config.port = networkObject.value(QStringLiteral("port")).toInt(config.port);
		config.theme = appearanceObject.value(QStringLiteral("theme")).toString(config.theme);
		config.lastPayload = coreObject.value(QStringLiteral("lastPayload")).toString(config.lastPayload);
		config.configFilePath = coreObject.value(QStringLiteral("configFilePath")).toString(config.configFilePath);
		if (config.configFilePath.isEmpty()) {
			config.configFilePath = defaultConfigFilePath();
		}
		return config;
	}

	static AppConfig load(const QString &filePath = defaultConfigFilePath())
	{
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly)) {
			AppConfig config;
			config.configFilePath = filePath;
			return config;
		}

		const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
		AppConfig config = fromJson(document.object());
		config.configFilePath = filePath;
		return config;
	}

	bool save(const QString &filePath = QString()) const
	{
		const QString finalPath = filePath.isEmpty() ? configFilePath : filePath;
		QFile file(finalPath);
		const QFileInfo fileInfo(file);
		QDir().mkpath(fileInfo.path());
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			return false;
		}
		file.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
		return true;
	}
};
