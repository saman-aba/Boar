#include "forgedpacketrepository.h"

#include <QByteArray>
#include <QDir>
#include <QStandardPaths>

#include <cstring>

namespace
{
QString defaultDatabasePath()
{
	const QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	return QDir(basePath).filePath(QStringLiteral("boar_packets.sqlite3"));
}

char *dupUtf8(const QString &value)
{
	const QByteArray utf8 = value.toUtf8();
	char *copy = new char[utf8.size() + 1];
	memcpy(copy, utf8.constData(), static_cast<size_t>(utf8.size()));
	copy[utf8.size()] = '\0';
	return copy;
}

unsigned char *dupBytes(const QByteArray &value)
{
	if (value.isEmpty()) {
		return nullptr;
	}
	auto *copy = new unsigned char[static_cast<size_t>(value.size())];
	memcpy(copy, value.constData(), static_cast<size_t>(value.size()));
	return copy;
}
}

ForgedPacketRepository::ForgedPacketRepository()
{
	const QByteArray path = defaultDatabasePath().toUtf8();
	database_ = boar_db_create(path.constData());
}

ForgedPacketRepository::~ForgedPacketRepository()
{
	if (database_ != nullptr) {
		boar_db_destroy(database_);
		database_ = nullptr;
	}
}

QList<ForgedPacketRecord> ForgedPacketRepository::packets() const
{
	return packets_;
}

void ForgedPacketRepository::setPackets(const QList<ForgedPacketRecord> &packets)
{
	packets_ = packets;
}

bool ForgedPacketRepository::upsertPacket(const ForgedPacketRecord &packet)
{
	boar_db_packet_record dbPacket = toDbRecord(packet);
	const bool ok = database_ != nullptr && boar_db_open(database_) && boar_db_upsert_packet(database_, &dbPacket);
	freeDbRecord(dbPacket);
	if (!ok) {
		return false;
	}
	for (auto &existingPacket : packets_) {
		if (existingPacket.id == packet.id) {
			existingPacket = packet;
			return true;
		}
	}
	packets_.prepend(packet);
	return true;
}

bool ForgedPacketRepository::removePacket(const QString &packetId)
{
	for (auto it = packets_.begin(); it != packets_.end(); ++it) {
		if (it->id == packetId) {
			const QByteArray id = packetId.toUtf8();
			if (database_ == nullptr || !boar_db_open(database_) || !boar_db_remove_packet(database_, id.constData())) {
				return false;
			}
			packets_.erase(it);
			return true;
		}
	}
	return false;
}

ForgedPacketRecord ForgedPacketRepository::packetById(const QString &packetId) const
{
	for (const auto &packet : packets_) {
		if (packet.id == packetId) {
			return packet;
		}
	}
	return {};
}

bool ForgedPacketRepository::contains(const QString &packetId) const
{
	for (const auto &packet : packets_) {
		if (packet.id == packetId) {
			return true;
		}
	}
	return false;
}

bool ForgedPacketRepository::load()
{
	boar_db_packet_list packetList {};
	if (database_ == nullptr || !boar_db_open(database_) || !boar_db_load_packets(database_, &packetList)) {
		return false;
	}
	QList<ForgedPacketRecord> loadedPackets;
	for (int index = 0; index < packetList.count; ++index) {
		loadedPackets.append(fromDbRecord(packetList.records[index]));
	}
	boar_db_packet_list_free(&packetList);
	packets_ = loadedPackets;
	return true;
}

QString ForgedPacketRepository::storagePath() const
{
	return database_ == nullptr ? QString() : QString::fromUtf8(boar_db_path(database_));
}

QString ForgedPacketRepository::lastError() const
{
	return database_ == nullptr ? QString() : QString::fromUtf8(boar_db_last_error(database_));
}

ForgedPacketRecord ForgedPacketRepository::fromDbRecord(const boar_db_packet_record &record)
{
	ForgedPacketRecord packet;
	packet.id = QString::fromUtf8(record.id == nullptr ? "" : record.id);
	packet.name = QString::fromUtf8(record.name == nullptr ? "" : record.name);
	packet.protocol = QString::fromUtf8(record.protocol == nullptr ? "" : record.protocol);
	packet.summary = QString::fromUtf8(record.summary == nullptr ? "" : record.summary);
	packet.exportBaseName = QString::fromUtf8(record.export_base_name == nullptr ? "" : record.export_base_name);
	packet.createdAt = QString::fromUtf8(record.created_at == nullptr ? "" : record.created_at);
	packet.sourceMac = QString::fromUtf8(record.source_mac == nullptr ? "" : record.source_mac);
	packet.destinationMac = QString::fromUtf8(record.destination_mac == nullptr ? "" : record.destination_mac);
	packet.sourceIp = QString::fromUtf8(record.source_ip == nullptr ? "" : record.source_ip);
	packet.destinationIp = QString::fromUtf8(record.destination_ip == nullptr ? "" : record.destination_ip);
	packet.transportProtocol = QString::fromUtf8(record.transport_protocol == nullptr ? "" : record.transport_protocol);
	packet.sourcePort = record.source_port;
	packet.destinationPort = record.destination_port;
	packet.sctpVerificationTag = QString::fromUtf8(record.sctp_tag == nullptr ? "" : record.sctp_tag);
	packet.packetTypeKey = QString::fromUtf8(record.packet_type_key == nullptr ? "" : record.packet_type_key);
	packet.payload = QByteArray(reinterpret_cast<const char *>(record.payload), static_cast<int>(record.payload_size));
	packet.frameBytes = QByteArray(reinterpret_cast<const char *>(record.frame_bytes), static_cast<int>(record.frame_bytes_size));
	packet.pcapBytes = QByteArray(reinterpret_cast<const char *>(record.pcap_bytes), static_cast<int>(record.pcap_bytes_size));
	return packet;
}

boar_db_packet_record ForgedPacketRepository::toDbRecord(const ForgedPacketRecord &record)
{
	boar_db_packet_record packet {};
	packet.id = dupUtf8(record.id);
	packet.name = dupUtf8(record.name);
	packet.protocol = dupUtf8(record.protocol);
	packet.summary = dupUtf8(record.summary);
	packet.export_base_name = dupUtf8(record.exportBaseName);
	packet.created_at = dupUtf8(record.createdAt);
	packet.source_mac = dupUtf8(record.sourceMac);
	packet.destination_mac = dupUtf8(record.destinationMac);
	packet.source_ip = dupUtf8(record.sourceIp);
	packet.destination_ip = dupUtf8(record.destinationIp);
	packet.transport_protocol = dupUtf8(record.transportProtocol);
	packet.source_port = record.sourcePort;
	packet.destination_port = record.destinationPort;
	packet.sctp_tag = dupUtf8(record.sctpVerificationTag);
	packet.packet_type_key = dupUtf8(record.packetTypeKey);
	packet.payload = dupBytes(record.payload);
	packet.payload_size = static_cast<size_t>(record.payload.size());
	packet.frame_bytes = dupBytes(record.frameBytes);
	packet.frame_bytes_size = static_cast<size_t>(record.frameBytes.size());
	packet.pcap_bytes = dupBytes(record.pcapBytes);
	packet.pcap_bytes_size = static_cast<size_t>(record.pcapBytes.size());
	return packet;
}

void ForgedPacketRepository::freeDbRecord(boar_db_packet_record &record)
{
	delete[] record.id;
	delete[] record.name;
	delete[] record.protocol;
	delete[] record.summary;
	delete[] record.export_base_name;
	delete[] record.created_at;
	delete[] record.source_mac;
	delete[] record.destination_mac;
	delete[] record.source_ip;
	delete[] record.destination_ip;
	delete[] record.transport_protocol;
	delete[] record.sctp_tag;
	delete[] record.packet_type_key;
	delete[] record.payload;
	delete[] record.frame_bytes;
	delete[] record.pcap_bytes;
	record = {};
}
