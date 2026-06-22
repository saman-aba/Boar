#pragma once

#include <QWidget>

#include <QByteArray>

#include <memory>

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

private:
	void populateHexTable(const QByteArray &bytes);
	QByteArray parseHexEditorText() const;
	void loadHexFromFile();
	void exportPcap();
	QByteArray buildExportFrame(const QByteArray &payload, const ExportPcapDialog &dialog) const;
	static QByteArray parseHexBytes(const QString &text, int expectedSize = -1);
	void saveTemplate();
	void saveTemplateAs();
	void syncHexEditorFromBuffer();
	void validateHexEditor();
	void applyHexEditorToActiveProtocol();
	void closeCurrentTemplate();
	void showStatusMessage(const QString &message) const;
	bool syncingHexView_ = false;
	QByteArray currentPacketBytes_;
	std::unique_ptr<Ui::PacketEditorModuleWidget> ui_;
	std::unique_ptr<DiameterEditorWidget> diameterEditorWidget_;
	std::unique_ptr<TcapEditorWidget> tcapEditorWidget_;
};
