#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QJsonObject>
#include <QString>

struct ForgedPacketRecord
{
	QString id;
	QString name;
	QString protocol;
	QString summary;
	QString exportBaseName;
	QString createdAt;
	QString sourceMac;
	QString destinationMac;
	QString sourceIp;
	QString destinationIp;
	QString transportProtocol;
	int sourcePort = -1;
	int destinationPort = -1;
	QString sctpVerificationTag;
	QString packetTypeKey;
	QByteArray payload;
	QByteArray frameBytes;
	QByteArray pcapBytes;

	[[nodiscard]] QJsonObject toJson() const;
	static ForgedPacketRecord fromJson(const QJsonObject &object);
	[[nodiscard]] int payloadSize() const;
};
