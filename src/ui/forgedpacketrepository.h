#pragma once

#include <QList>
#include <QString>

#include "../core/db.h"
#include "forgedpacketrecord.h"

class ForgedPacketRepository
{
public:
	ForgedPacketRepository();
	~ForgedPacketRepository();

	[[nodiscard]] QList<ForgedPacketRecord> packets() const;
	void setPackets(const QList<ForgedPacketRecord> &packets);
	bool upsertPacket(const ForgedPacketRecord &packet);
	bool removePacket(const QString &packetId);
	[[nodiscard]] ForgedPacketRecord packetById(const QString &packetId) const;
	[[nodiscard]] bool contains(const QString &packetId) const;
	[[nodiscard]] bool load();
	[[nodiscard]] QString storagePath() const;
	[[nodiscard]] QString lastError() const;

private:
	[[nodiscard]] static ForgedPacketRecord fromDbRecord(const boar_db_packet_record &record);
	[[nodiscard]] static boar_db_packet_record toDbRecord(const ForgedPacketRecord &record);
	static void freeDbRecord(boar_db_packet_record &record);

	QList<ForgedPacketRecord> packets_;
	boar_db *database_ = nullptr;
};
