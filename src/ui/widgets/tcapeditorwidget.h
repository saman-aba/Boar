#pragma once

#include <QByteArray>
#include <QWidget>

#include <functional>
#include <memory>

class QString;

namespace Ui
{
class TcapEditorWidget;
}

class TcapEditorWidget : public QWidget
{
public:
	explicit TcapEditorWidget(QWidget *parent = nullptr);
	~TcapEditorWidget() override;

	QByteArray encodeCurrentMessage(QString *errorMessage = nullptr) const;
	QByteArray currentTemplateBuffer() const;
	QByteArray loadedTemplateBuffer() const;
	QString currentTemplatePath() const;
	bool saveTemplate();
	bool saveTemplateAs(const QString &filePath);
	void openTemplateFileDialog();
	void setTemplateBuffer(const QByteArray &buffer);
	void setOnPacketChanged(const std::function<void(const QByteArray &)> &onPacketChanged);

private:
	void wireSignals();
	void previewStructure();
	void previewHex();
	void validateInput();
	QString decodeCurrentBuffer() const;
	QByteArray buildFallbackTcapBytes() const;
	QByteArray buildTemplateJson() const;
	void loadTemplateJson(const QByteArray &buffer);
	void populateGeneratedDropdowns();
	void refreshOperationOptions();
	QStringList parameterOptionsForCurrentOperation() const;
	QStringList parameterOptionsForOperation(const QString &family, const QString &operation) const;
	void addParameter(bool asChild);
	void moveCurrentParameter(int delta);
	QString buildStructureText() const;
	static QByteArray parseHexField(const QString &text);

	std::unique_ptr<Ui::TcapEditorWidget> ui_;
	QByteArray currentBuffer_;
	QString selectedTemplatePath_;
	QByteArray loadedTemplateBuffer_;
	std::function<void(const QByteArray &)> onPacketChanged_ {};
};
