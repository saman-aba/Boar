#include "diametereditorwidget.h"

#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHeaderView>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <arpa/inet.h>

extern "C" {
#include "../../core/diameter.h"
}

#include "../dialogs/editavpdialog.h"
#include "ui_DiameterEditorWidget.h"

DiameterEditorWidget::DiameterEditorWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::DiameterEditorWidget>())
{
	ui_->setupUi(this);
	if (ui_->treeAvps) {
		ui_->treeAvps->setDragDropMode(QAbstractItemView::InternalMove);
		ui_->treeAvps->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		if (auto *header = ui_->treeAvps->header()) {
			header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
			header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
			header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
			header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
			header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
			header->setSectionResizeMode(5, QHeaderView::Interactive);
		}
		ui_->treeAvps->setColumnWidth(5, 180);
		connect(ui_->treeAvps, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item) {
			if (!item) {
				return;
			}
			EditAvpDialog dialog(item, this);
			if (dialog.exec() != QDialog::Accepted) {
				return;
			}
			item->setText(0, dialog.avpName());
			item->setText(1, QString::number(dialog.avpCode()));
			item->setText(2, dialog.avpFlags().contains(QStringLiteral("V")) ?
					QStringLiteral("✓") :
					QStringLiteral(""));

			item->setText(3, dialog.avpFlags().contains(QStringLiteral("M")) ?
					QStringLiteral("✓") :
					QStringLiteral(""));

			item->setText(4, dialog.avpFlags().contains(QStringLiteral("P"))
					? QStringLiteral("✓") :
					QStringLiteral(""));

			item->setText(5, formatAvpValue(dialog.avpType(),
						dialog.avpDerivedType(),
						dialog.avpValue()));

			item->setData(0, Qt::UserRole, dialog.avpType());
			item->setData(0, Qt::UserRole + 1, dialog.avpDerivedType());
			item->setData(0, Qt::UserRole + 2, dialog.avpValue());
			notifyTemplateChanged();
		});
	}
	if (ui_->btnAddAvp) {
		connect(ui_->btnAddAvp, &QPushButton::clicked, this, [this]() {
			auto *item = new QTreeWidgetItem();
			item->setText(0, QStringLiteral("New AVP"));
			item->setText(1, QStringLiteral("0"));
			item->setText(2, QStringLiteral(""));
			item->setText(3, QStringLiteral(""));
			item->setText(4, QStringLiteral(""));
			item->setText(5, QStringLiteral(""));
			item->setData(0, Qt::UserRole, QStringLiteral("OctetString"));
			item->setData(0, Qt::UserRole + 1, QStringLiteral("UTF8String"));
			item->setData(0, Qt::UserRole + 2, QStringLiteral(""));
			EditAvpDialog dialog(item, this);
			if (dialog.exec() != QDialog::Accepted) {
				delete item;
				return;
			}
			item->setText(0, dialog.avpName());
			item->setText(1, QString::number(dialog.avpCode()));

			item->setText(2, dialog.avpFlags().contains(QStringLiteral("V")) ?
					QStringLiteral("✓") :
					QStringLiteral(""));

			item->setText(3, dialog.avpFlags().contains(QStringLiteral("M")) ?
					QStringLiteral("✓") :
					QStringLiteral(""));

			item->setText(4, dialog.avpFlags().contains(QStringLiteral("P")) ?
					QStringLiteral("✓") :
					QStringLiteral(""));

			item->setText(5, formatAvpValue(dialog.avpType(),
					dialog.avpDerivedType(),
					dialog.avpValue()));

			item->setData(0, Qt::UserRole, dialog.avpType());
			item->setData(0, Qt::UserRole + 1, dialog.avpDerivedType());
			item->setData(0, Qt::UserRole + 2, dialog.avpValue());

			insertAvpItemAfterSelection(item);
			notifyTemplateChanged();
		});
	}
	if (ui_->btnRemoveAvp) {
		connect(ui_->btnRemoveAvp, &QPushButton::clicked, this, [this]() {
			if (!ui_->treeAvps || !ui_->treeAvps->currentItem()) {
				return;
			}
			auto *item = ui_->treeAvps->currentItem();
			if (auto *parent = item->parent()) {
				delete parent->takeChild(parent->indexOfChild(item));
			} else {
				delete ui_->treeAvps->takeTopLevelItem(ui_->treeAvps->indexOfTopLevelItem(item));
			}
			notifyTemplateChanged();
		});
	}
	if (ui_->btnAutoPopulate) {
		connect(ui_->btnAutoPopulate,
				&QPushButton::clicked,
				this,
				[this]() {
					selectTemplateFile();
				}
		);
	}
}

void DiameterEditorWidget::selectTemplateFile()
{
	const QString filePath = QFileDialog::getOpenFileName(this,
		QStringLiteral("Select Diameter Template"),
		selectedTemplatePath_.isEmpty() ? QString() : selectedTemplatePath_,
		QStringLiteral("JSON Templates (*.json);;All Files (*)"));
	if (filePath.isEmpty()) {
		return;
	}
	selectedTemplatePath_ = filePath;
	if (!loadTemplateBuffer(filePath)) {
		return;
	}
	if (ui_->btnAutoPopulate) {
		ui_->btnAutoPopulate->setToolTip(filePath);
	}
	if (loadedTemplateBuffer_.isEmpty()) {
		loadedBytes_.clear();
		return;
	}
	struct diameter_pkt *packet = diameter_read_json_packet(loadedTemplateBuffer_.constData());
	if (!packet) {
		loadedBytes_.clear();
		return;
	}
	loadedBytes_ = serializeTemplateBuffer(loadedTemplateBuffer_);
	populateFromPacket(packet);
	diameter_packet_free(packet);
	if (onTemplateChanged_) {
		onTemplateChanged_(loadedBytes_);
	}
}

bool DiameterEditorWidget::loadTemplateBuffer(const QString &filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		loadedTemplateBuffer_.clear();
		return false;
	}
	loadedTemplateBuffer_ = file.readAll();
	return !loadedTemplateBuffer_.isEmpty();
}

QByteArray DiameterEditorWidget::serializeTemplateBuffer(const QByteArray &buffer) const
{
	if (buffer.isEmpty()) {
		return {};
	}
	struct diameter_pkt *packet = diameter_read_json_packet(buffer.constData());
	if (!packet) {
		return {};
	}
	QByteArray serializedBytes(65535, Qt::Uninitialized);
	const int serializedSize = diameter_serialize_packet(
			packet,
			reinterpret_cast<uint8_t *>(serializedBytes.data()));
	diameter_packet_free(packet);
	if (serializedSize <= 0 || serializedSize > serializedBytes.size()) {
		return {};
	}
	serializedBytes.resize(serializedSize);
	return serializedBytes;
}

QString DiameterEditorWidget::formatAvpFlags(unsigned char flags) const
{
	QString text;
	if (flags & AVP_FLAG_VENDOR) {
		text += QStringLiteral("V");
	}
	if (flags & AVP_FLAG_MANDATORY) {
		text += QStringLiteral("M");
	}
	if (flags & AVP_FLAG_PROTECTED) {
		text += QStringLiteral("P");
	}
	return text;
}

QString DiameterEditorWidget::displayAvpName(unsigned int code) const
{
	return QStringLiteral("AVP %1").arg(code);
}

QString DiameterEditorWidget::formatAvpValue(const QString &type,
		const QString &derivedType,
		const QString &rawValue) const
{
	if (type == QStringLiteral("OctetString") && derivedType != QStringLiteral("UTF8String")
		&& derivedType != QStringLiteral("DiameterIdentity")
		&& derivedType != QStringLiteral("DiameterURI")
		&& derivedType != QStringLiteral("IPFilterRule")
		&& derivedType != QStringLiteral("QoSFilterRule")) {
		return QByteArray(rawValue.toUtf8()).toHex(' ');
	}
	return rawValue;
}

void DiameterEditorWidget::populateAvpTree(const diameter_avp *avps,
		const QJsonArray &descriptorAvps,
		QTreeWidgetItem *parentItem)
{
	int descriptorIndex = 0;
	for (auto *avp = avps; avp; avp = avp->next, ++descriptorIndex) {
		auto *item = new QTreeWidgetItem();
		const QJsonObject descriptorObject = descriptorAvps.at(descriptorIndex).toObject();
		const QString descriptorName = descriptorObject.value(QStringLiteral("name")).toString();
		const QString descriptorType = descriptorObject.value(QStringLiteral("type")).toString();

		item->setText(0, descriptorName.isEmpty() ?
				displayAvpName(AVP_HEADER(avp).code) :
				descriptorName);
		item->setText(1, QString::number(AVP_HEADER(avp).code));

		item->setText(2, (AVP_HEADER(avp).flags & AVP_FLAG_VENDOR) ?
				QStringLiteral("✓") :
				QStringLiteral(""));

		item->setText(3, (AVP_HEADER(avp).flags & AVP_FLAG_MANDATORY) ?
				QStringLiteral("✓") :
				QStringLiteral(""));

		item->setText(4, (AVP_HEADER(avp).flags & AVP_FLAG_PROTECTED) ?
				QStringLiteral("✓") :
				QStringLiteral(""));

		switch (avp->type) {
		case OctetString: {
			const QString rawValue = avp->data.octetstring ?
					QString::fromUtf8(avp->data.octetstring) :
					QString();

			const QString effectiveType = descriptorType.isEmpty() ?
					QStringLiteral("OctetString") :
					descriptorType;

			const QString displayValue = formatAvpValue(QStringLiteral("OctetString"), effectiveType, rawValue);
			item->setText(5, displayValue);
			item->setData(0, Qt::UserRole, QStringLiteral("OctetString"));
			item->setData(0, Qt::UserRole + 1, effectiveType);

			item->setData(0, Qt::UserRole + 2, displayValue == rawValue ?
					rawValue :
					displayValue);
			break;
		}
		case Integer32: {
			const QString rawValue = QString::number(ntohl(avp->data.int32));
			item->setText(5, rawValue);
			item->setData(0, Qt::UserRole, QStringLiteral("Integer32"));
			item->setData(0, Qt::UserRole + 2, rawValue);
			break;
		}
		case Integer64: {
			const QString rawValue = QString::number(static_cast<qlonglong>(be64toh(avp->data.int64)));
			item->setText(5, rawValue);
			item->setData(0, Qt::UserRole, QStringLiteral("Integer64"));
			item->setData(0, Qt::UserRole + 2, rawValue);
			break;
		}
		case Unsigned32: {
			const QString rawValue = QString::number(ntohl(avp->data.unsigned32));
			item->setText(5, rawValue);
			item->setData(0, Qt::UserRole, QStringLiteral("Unsigned32"));
			item->setData(0, Qt::UserRole + 2, rawValue);
			break;
		}
		case Unsigned64: {
			const QString rawValue = QString::number(static_cast<qulonglong>(be64toh(avp->data.unsigned64)));
			item->setText(5, rawValue);
			item->setData(0, Qt::UserRole, QStringLiteral("Unsigned64"));
			item->setData(0, Qt::UserRole + 2, rawValue);
			break;
		}
		case Grouped:
			item->setText(5, QStringLiteral("Grouped"));
			item->setData(0, Qt::UserRole, QStringLiteral("Grouped"));
			item->setData(0, Qt::UserRole + 2, QStringLiteral(""));
			break;
		default:
			item->setData(0, Qt::UserRole, QStringLiteral("Unknown"));
			break;
		}
		if (parentItem) {
			parentItem->addChild(item);
		} else if (ui_->treeAvps) {
			ui_->treeAvps->addTopLevelItem(item);
		}
		if (avp->type == Grouped && avp->data.group) {
			populateAvpTree(static_cast<const diameter_avp *>(avp->data.group),
					descriptorObject.value(QStringLiteral("value")).toArray(), item);
		}
	}
}

bool DiameterEditorWidget::populateFromPacket(const diameter_pkt *packet)
{
	if (!packet) {
		return false;
	}
	if (ui_->comboCommand) {
		const QString commandCode = QString::number(packet->header.command_code);
		if (ui_->comboCommand->findText(commandCode) == -1) {
			ui_->comboCommand->addItem(commandCode);
		}
		ui_->comboCommand->setCurrentText(commandCode);
	}
	if (ui_->editAppId) {
		ui_->editAppId->setText(QString::number(packet->header.application_id));
	}
	if (ui_->editHopByHopId) {
		ui_->editHopByHopId->setText(QString::number(packet->header.hop_by_hop_id));
	}
	if (ui_->editEndToEndId) {
		ui_->editEndToEndId->setText(QString::number(packet->header.end_to_end_id));
	}
	if (ui_->checkRequest) {
		ui_->checkRequest->setChecked((packet->header.flags & FLAG_REQUEST) != 0);
	}
	if (ui_->checkProxiable) {
		ui_->checkProxiable->setChecked((packet->header.flags & FLAG_PROXYABLE) != 0);
	}
	if (ui_->checkError) {
		ui_->checkError->setChecked((packet->header.flags & FLAG_ERROR) != 0);
	}

	if (ui_->treeAvps) {
		ui_->treeAvps->clear();
		const QJsonDocument document = QJsonDocument::fromJson(loadedTemplateBuffer_);

		const QJsonArray descriptorAvps = document
				.object()
				.value(QStringLiteral("diameter"))
				.toObject()
				.value(QStringLiteral("avps"))
				.toArray();

		populateAvpTree(packet->avp_list, descriptorAvps, nullptr);
		ui_->treeAvps->expandAll();
	}
	return true;
}

QByteArray DiameterEditorWidget::loadedTemplateBuffer() const
{
	return loadedTemplateBuffer_;
}

void DiameterEditorWidget::setTemplateBuffer(const QByteArray &buffer)
{
	loadedTemplateBuffer_ = buffer;
	loadedBytes_.clear();
	if (loadedTemplateBuffer_.isEmpty()) {
		return;
	}
	struct diameter_pkt *packet = diameter_read_json_packet(loadedTemplateBuffer_.constData());
	if (!packet) {
		return;
	}
	loadedBytes_ = serializeTemplateBuffer(loadedTemplateBuffer_);
	populateFromPacket(packet);
	diameter_packet_free(packet);
}

void DiameterEditorWidget::clearTemplate()
{
	selectedTemplatePath_.clear();
	loadedTemplateBuffer_.clear();
	loadedBytes_.clear();
	if (ui_->comboCommand) {
		ui_->comboCommand->setCurrentIndex(-1);
	}
	if (ui_->editAppId) {
		ui_->editAppId->clear();
	}
	if (ui_->editHopByHopId) {
		ui_->editHopByHopId->clear();
	}
	if (ui_->editEndToEndId) {
		ui_->editEndToEndId->clear();
	}
	if (ui_->checkRequest) {
		ui_->checkRequest->setChecked(false);
	}
	if (ui_->checkProxiable) {
		ui_->checkProxiable->setChecked(false);
	}
	if (ui_->checkError) {
		ui_->checkError->setChecked(false);
	}
	if (ui_->treeAvps) {
		ui_->treeAvps->clear();
	}
	if (ui_->btnAutoPopulate) {
		ui_->btnAutoPopulate->setToolTip(QString());
	}
}

void DiameterEditorWidget::setOnTemplateChanged(const std::function<void(const QByteArray &)> &onTemplateChanged)
{
	onTemplateChanged_ = onTemplateChanged;
}

QByteArray DiameterEditorWidget::currentLoadedBytes() const
{
	return loadedBytes_;
}

QByteArray DiameterEditorWidget::currentTemplateBuffer() const
{
	QJsonObject diameterObject;
	diameterObject.insert(QStringLiteral("common-header"), buildHeaderObject());
	diameterObject.insert(QStringLiteral("avps"), buildAvpArray(nullptr));
	QJsonObject rootObject;
	rootObject.insert(QStringLiteral("diameter"), diameterObject);
	return QJsonDocument(rootObject).toJson(QJsonDocument::Indented);
}

QString DiameterEditorWidget::currentTemplatePath() const
{
	return selectedTemplatePath_;
}

bool DiameterEditorWidget::saveTemplate()
{
	if (selectedTemplatePath_.isEmpty()) {
		return false;
	}
	return saveTemplateAs(selectedTemplatePath_);
}

bool DiameterEditorWidget::saveTemplateAs(const QString &filePath)
{
	if (filePath.isEmpty()) {
		return false;
	}
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		return false;
	}
	const QByteArray buffer = currentTemplateBuffer();
	if (file.write(buffer) != buffer.size()) {
		return false;
	}
	selectedTemplatePath_ = filePath;
	loadedTemplateBuffer_ = buffer;
	if (ui_->btnAutoPopulate) {
		ui_->btnAutoPopulate->setToolTip(filePath);
	}
	notifyTemplateChanged();
	return true;
}

void DiameterEditorWidget::insertAvpItemAfterSelection(QTreeWidgetItem *item)
{
	if (!ui_->treeAvps || !item) {
		return;
	}
	auto *current = ui_->treeAvps->currentItem();
	if (!current) {
		ui_->treeAvps->addTopLevelItem(item);
		ui_->treeAvps->setCurrentItem(item);
		return;
	}
	if (auto *parent = current->parent()) {
		parent->insertChild(parent->indexOfChild(current) + 1, item);
	} else {
		ui_->treeAvps->insertTopLevelItem(ui_->treeAvps->indexOfTopLevelItem(current) + 1, item);
	}
	ui_->treeAvps->setCurrentItem(item);
}

void DiameterEditorWidget::notifyTemplateChanged()
{
	loadedTemplateBuffer_ = currentTemplateBuffer();
	loadedBytes_ = serializeTemplateBuffer(loadedTemplateBuffer_);
	if (onTemplateChanged_) {
		onTemplateChanged_(loadedBytes_);
	}
}

QJsonObject DiameterEditorWidget::buildHeaderObject() const
{
	QJsonObject headerObject;
	headerObject.insert(QStringLiteral("version"), 1);
	QString flags;

	if (ui_->checkRequest && ui_->checkRequest->isChecked()) {
		flags += QStringLiteral("R");
	}

	if (ui_->checkProxiable && ui_->checkProxiable->isChecked()) {
		flags += QStringLiteral("P");
	}

	if (ui_->checkError && ui_->checkError->isChecked()) {
		flags += QStringLiteral("E");
	}

	headerObject.insert(QStringLiteral("flags"),
			flags);

	headerObject.insert(QStringLiteral("command-code"),
			ui_->comboCommand ?
			ui_->comboCommand->currentText().toInt() :
			0);

	headerObject.insert(QStringLiteral("application-id"),
			ui_->editAppId ?
			ui_->editAppId->text().toInt() :
			0);

	headerObject.insert(QStringLiteral("hop-by-hop-id"),
			static_cast<qint64>(ui_->editHopByHopId ?
			ui_->editHopByHopId->text().toUInt() :
			0));

	headerObject.insert(QStringLiteral("end-to-end-id"),
			static_cast<qint64>(ui_->editEndToEndId ?
			ui_->editEndToEndId->text().toUInt() :
			0));

	return headerObject;
}

QJsonObject DiameterEditorWidget::buildAvpObject(const QTreeWidgetItem *item) const
{
	QJsonObject avpObject;
	avpObject.insert(QStringLiteral("name"), item->text(0));
	avpObject.insert(QStringLiteral("code"), item->text(1).toInt());
	QString flags;

	if (item->text(2) == QStringLiteral("✓")) {
		flags += QStringLiteral("V");
	}

	if (item->text(3) == QStringLiteral("✓")) {
		flags += QStringLiteral("M");
	}

	if (item->text(4) == QStringLiteral("✓")) {
		flags += QStringLiteral("P");
	}

	avpObject.insert(QStringLiteral("flags"), flags);
	avpObject.insert(QStringLiteral("type"),
			item->data(0, Qt::UserRole + 1).toString().isEmpty() ?
			item->data(0, Qt::UserRole).toString() :
			item->data(0, Qt::UserRole + 1).toString());

	if (item->data(0, Qt::UserRole).toString() == QStringLiteral("Grouped")) {
		avpObject.insert(QStringLiteral("value"), buildAvpArray(item));
	} else {
		const QString rawValue = item->data(0, Qt::UserRole + 2).toString();
		bool ok = false;
		const qlonglong numericValue = rawValue.toLongLong(&ok);
		if (ok && item->data(0, Qt::UserRole).toString() != QStringLiteral("OctetString")) {
			avpObject.insert(QStringLiteral("value"), numericValue);
		} else {
			avpObject.insert(QStringLiteral("value"), rawValue);
		}
	}
	return avpObject;
}

QJsonArray DiameterEditorWidget::buildAvpArray(const QTreeWidgetItem *parentItem) const
{
	QJsonArray avpArray;
	if (parentItem) {
		for (int index = 0; index < parentItem->childCount(); ++index) {
			avpArray.append(buildAvpObject(parentItem->child(index)));
		}
		return avpArray;
	}
	if (!ui_->treeAvps) {
		return avpArray;
	}
	for (int index = 0; index < ui_->treeAvps->topLevelItemCount(); ++index) {
		avpArray.append(buildAvpObject(ui_->treeAvps->topLevelItem(index)));
	}
	return avpArray;
}

void DiameterEditorWidget::openTemplateFileDialog()
{
	selectTemplateFile();
}

DiameterEditorWidget::~DiameterEditorWidget() = default;
