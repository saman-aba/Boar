#pragma once

#include <QWidget>

#include <QByteArray>

#include <functional>
#include <memory>

#include "../forgedpacketrecord.h"

namespace Ui
{
class PacketEditorModuleWidget;
}

class DiameterEditorWidget;
class ExportPcapDialog;
class TcapEditorWidget;

class PacketEditorModuleWidget : public QWidget
{
public:
	explicit PacketEditorModuleWidget(QWidget *parent = nullptr);
	~PacketEditorModuleWidget() override;

	void setOnPacketForged(const std::function<void(const ForgedPacketRecord &)> &callback);
	void loadPacket(const ForgedPacketRecord &packet);
	void clearEditingPacket();

private:
	void populateHexTable(const QByteArray &bytes);
	QByteArray parseHexEditorText() const;
	void loadHexFromFile();
	void exportPcap();
	void injectPacket();
	QByteArray buildExportFrame(const QByteArray &payload, const ExportPcapDialog &dialog) const;
	QByteArray buildPcapBytes(const QByteArray &frameBytes) const;
	ForgedPacketRecord buildForgedPacketRecord(const QByteArray &payload, const QByteArray &frameBytes, const QByteArray &pcapBytes, const QString &name, const QString &summary) const;
	void populatePacketMetadata(ForgedPacketRecord &packet, const QByteArray &frameBytes) const;
	[[nodiscard]] QString packetTypeKey() const;
	static QByteArray parseHexBytes(const QString &text, int expectedSize = -1);
	static QString formatMacAddress(const QByteArray &bytes);
	static QString formatIpv4Address(const QByteArray &bytes);
	void saveTemplate();
	void saveTemplateAs();
	void syncHexEditorFromBuffer();
	void validateHexEditor();
	void applyHexEditorToActiveProtocol();
	void closeCurrentTemplate();
	void showStatusMessage(const QString &message) const;
	QString activeProtocolName() const;
	bool syncingHexView_ = false;
	QByteArray currentPacketBytes_;
	QString editingPacketId_;
	std::function<void(const ForgedPacketRecord &)> onPacketForged_ {};
	std::unique_ptr<Ui::PacketEditorModuleWidget> ui_;
	std::unique_ptr<DiameterEditorWidget> diameterEditorWidget_;
	std::unique_ptr<TcapEditorWidget> tcapEditorWidget_;
};
