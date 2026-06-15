#pragma once

#include <QWidget>

#include <QByteArray>

#include <memory>

namespace Ui
{
class PacketEditorModuleWidget;
}

class DiameterEditorWidget;
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
