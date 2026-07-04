#include "forgedpacketrecord.h"

#include <QJsonValue>

QJsonObject ForgedPacketRecord::toJson() const
{
	QJsonObject object;
	object.insert(QStringLiteral("id"), id);
	object.insert(QStringLiteral("name"), name);
	object.insert(QStringLiteral("protocol"), protocol);
	object.insert(QStringLiteral("summary"), summary);
	object.insert(QStringLiteral("exportBaseName"), exportBaseName);
	object.insert(QStringLiteral("createdAt"), createdAt);
	object.insert(QStringLiteral("sourceMac"), sourceMac);
	object.insert(QStringLiteral("destinationMac"), destinationMac);
	object.insert(QStringLiteral("sourceIp"), sourceIp);
	object.insert(QStringLiteral("destinationIp"), destinationIp);
	object.insert(QStringLiteral("transportProtocol"), transportProtocol);
	object.insert(QStringLiteral("sourcePort"), sourcePort);
	object.insert(QStringLiteral("destinationPort"), destinationPort);
	object.insert(QStringLiteral("sctpVerificationTag"), sctpVerificationTag);
	object.insert(QStringLiteral("packetTypeKey"), packetTypeKey);
	object.insert(QStringLiteral("payload"), QString::fromLatin1(payload.toHex()));
	object.insert(QStringLiteral("frameBytes"), QString::fromLatin1(frameBytes.toHex()));
	object.insert(QStringLiteral("pcapBytes"), QString::fromLatin1(pcapBytes.toHex()));
	return object;
}

ForgedPacketRecord ForgedPacketRecord::fromJson(const QJsonObject &object)
{
	ForgedPacketRecord record;
	record.id = object.value(QStringLiteral("id")).toString();
	record.name = object.value(QStringLiteral("name")).toString();
	record.protocol = object.value(QStringLiteral("protocol")).toString();
	record.summary = object.value(QStringLiteral("summary")).toString();
	record.exportBaseName = object.value(QStringLiteral("exportBaseName")).toString();
	record.createdAt = object.value(QStringLiteral("createdAt")).toString();
	record.sourceMac = object.value(QStringLiteral("sourceMac")).toString();
	record.destinationMac = object.value(QStringLiteral("destinationMac")).toString();
	record.sourceIp = object.value(QStringLiteral("sourceIp")).toString();
	record.destinationIp = object.value(QStringLiteral("destinationIp")).toString();
	record.transportProtocol = object.value(QStringLiteral("transportProtocol")).toString();
	record.sourcePort = object.value(QStringLiteral("sourcePort")).toInt(-1);
	record.destinationPort = object.value(QStringLiteral("destinationPort")).toInt(-1);
	record.sctpVerificationTag = object.value(QStringLiteral("sctpVerificationTag")).toString();
	record.packetTypeKey = object.value(QStringLiteral("packetTypeKey")).toString();
	record.payload = QByteArray::fromHex(object.value(QStringLiteral("payload")).toString().toLatin1());
	record.frameBytes = QByteArray::fromHex(object.value(QStringLiteral("frameBytes")).toString().toLatin1());
	record.pcapBytes = QByteArray::fromHex(object.value(QStringLiteral("pcapBytes")).toString().toLatin1());
	return record;
}

int ForgedPacketRecord::payloadSize() const
{
	return payload.isEmpty() ? frameBytes.size() : payload.size();
}
