#pragma once

#include <QList>
#include <QWidget>

#include <functional>
#include <memory>

#include "../forgedpacketrecord.h"

class PacketExportDialog;

namespace Ui
{
class DashboardModuleWidget;
}

class DashboardModuleWidget : public QWidget
{
public:
	explicit DashboardModuleWidget(QWidget *parent = nullptr);
	~DashboardModuleWidget() override;

	void setPackets(const QList<ForgedPacketRecord> &packets);
	void addPacket(const ForgedPacketRecord &packet);
	void updatePacket(const ForgedPacketRecord &packet);
	void removePacket(const QString &packetId);
	void setOnOpenPacketGenerator(const std::function<void()> &callback);
	void setOnOpenPacket(const std::function<void(const ForgedPacketRecord &)> &callback);
	void setOnTransmitPacket(const std::function<void(const ForgedPacketRecord &, const QString &)> &callback);
	void setOnRemovePacket(const std::function<void(const QString &)> &callback);
	void setOnEditPacket(const std::function<void(const QString &)> &callback);
	void setOnExportPacket(const std::function<void(const ForgedPacketRecord &)> &callback);
	void appendLog(const QString &message);

private:
	void refreshPacketTree();
	ForgedPacketRecord currentPacket() const;
	void requestTransmitCurrentPacket();
	void requestExportCurrentPacket();
	void setPacketsModelSelection(const QString &packetId);

	QList<ForgedPacketRecord> packets_;
	std::function<void()> onOpenPacketGenerator_ {};
	std::function<void(const ForgedPacketRecord &)> onOpenPacket_ {};
	std::function<void(const ForgedPacketRecord &, const QString &)> onTransmitPacket_ {};
	std::function<void(const QString &)> onRemovePacket_ {};
	std::function<void(const QString &)> onEditPacket_ {};
	std::function<void(const ForgedPacketRecord &)> onExportPacket_ {};
	std::unique_ptr<Ui::DashboardModuleWidget> ui_;
};
