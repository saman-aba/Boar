#pragma once

#include <QWidget>

#include <memory>

#include <QByteArray>
#include <QString>

#include <functional>

class QJsonArray;
struct diameter_avp;
struct diameter_pkt;
class QJsonArray;
class QJsonObject;
class QTreeWidgetItem;

namespace Ui
{
class DiameterEditorWidget;
}

class DiameterEditorWidget : public QWidget
{
public:
	explicit DiameterEditorWidget(QWidget *parent = nullptr);
	~DiameterEditorWidget() override;
	void openTemplateFileDialog();
	QByteArray loadedTemplateBuffer() const;
	QByteArray currentTemplateBuffer() const;
	QByteArray currentLoadedBytes() const;
	QString currentTemplatePath() const;
	bool saveTemplate();
	bool saveTemplateAs(const QString &filePath);
	void setTemplateBuffer(const QByteArray &buffer);
	void clearTemplate();
	void setOnTemplateChanged(const std::function<void(const QByteArray &)> &onTemplateChanged);

private:
	void selectTemplateFile();
	bool loadTemplateBuffer(const QString &filePath);
	bool populateFromPacket(const diameter_pkt *packet);
	QByteArray serializeTemplateBuffer(const QByteArray &buffer) const;

	void populateAvpTree(const diameter_avp *avps,
			const QJsonArray &descriptorAvps,
			QTreeWidgetItem *parentItem);

	void insertAvpItemAfterSelection(QTreeWidgetItem *item);
	void notifyTemplateChanged();
	QJsonObject buildHeaderObject() const;
	QJsonObject buildAvpObject(const QTreeWidgetItem *item) const;
	QJsonArray buildAvpArray(const QTreeWidgetItem *parentItem) const;
	QString displayAvpName(unsigned int code) const;

	QString formatAvpValue(const QString &type,
			const QString &derivedType,
			const QString &rawValue) const;

	QString formatAvpFlags(unsigned char flags) const;

	std::unique_ptr<Ui::DiameterEditorWidget> ui_;

	QString selectedTemplatePath_;
	QByteArray loadedTemplateBuffer_;
	QByteArray loadedBytes_;

	std::function<void(const QByteArray &)> onTemplateChanged_ {};
};
