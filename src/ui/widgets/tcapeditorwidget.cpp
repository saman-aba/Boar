#include "tcapeditorwidget.h"

#include <QByteArray>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QRegularExpression>
#include <QStringList>
#include <QTreeWidgetItem>

#include "../dialogs/addtcapparameterdialog.h"

extern "C" {
#include "../../core/asn1.h"
#include "../../core/generated/tcap.h"
}
#include "ui_TcapEditorWidget.h"

namespace
{
QStringList loadOperationNames(const QString &path, const QRegularExpression &pattern)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return {};
	}
	const QString content = QString::fromUtf8(file.readAll());
	QStringList values;
	auto it = pattern.globalMatch(content);
	while (it.hasNext()) {
		const auto match = it.next();
		const QString value = match.lastCapturedIndex() >= 2 ? match.captured(2).trimmed() : match.captured(1).trimmed();
		if (!value.isEmpty() && !values.contains(value)) {
			values.append(value);
		}
	}
	return values;
}

QStringList loadParameterNames(const QString &path, const QString &enumPrefix)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return {};
	}
	const QString content = QString::fromUtf8(file.readAll());
	const QRegularExpression pattern(QStringLiteral("%1_([a-zA-Z0-9_]+),").arg(QRegularExpression::escape(enumPrefix)));
	QStringList values;
	auto it = pattern.globalMatch(content);
	while (it.hasNext()) {
		const auto match = it.next();
		values.append(match.captured(1));
	}
	values.removeDuplicates();
	return values;
}

QList<TcapParameterOption> loadStructParameterDetails(const QString &path, const QString &structName)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return {};
	}
	const QString content = QString::fromUtf8(file.readAll());
	const QRegularExpression blockPattern(QStringLiteral(R"(typedef struct %1 \{([^}]*)\} %1_t;)").arg(QRegularExpression::escape(structName)), QRegularExpression::DotMatchesEverythingOption);
	const auto blockMatch = blockPattern.match(content);
	if (!blockMatch.hasMatch()) {
		return {};
	}
	const QString block = blockMatch.captured(1);
	const QRegularExpression linePattern(QStringLiteral(R"(^\s*([A-Za-z0-9_]+(?:\s*\*)?)\s+([A-Za-z0-9_]+);)") , QRegularExpression::MultilineOption);
	QList<TcapParameterOption> values;
	auto it = linePattern.globalMatch(block);
	while (it.hasNext()) {
		const auto match = it.next();
		const QString fieldName = match.captured(2).trimmed();
		if (fieldName == QStringLiteral("seen_mask")) {
			continue;
		}
		QString typeName = match.captured(1).trimmed();
		typeName.replace(QStringLiteral(" *"), QStringLiteral("*"));
		values.append({fieldName, typeName});
	}
	return values;
}

QList<TcapParameterOption> loadSequenceParameterDetails(const QString &path, const QString &sequenceName)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return {};
	}
	const QString content = QString::fromUtf8(file.readAll());
	const QRegularExpression blockPattern(QStringLiteral(R"(%1 \{PARAMETERS-BOUND[^
]*::= SEQUENCE \{(.*?)
	\})").arg(QRegularExpression::escape(sequenceName)), QRegularExpression::DotMatchesEverythingOption);
	const auto blockMatch = blockPattern.match(content);
	if (!blockMatch.hasMatch()) {
		return {};
	}
	const QString block = blockMatch.captured(1);
	const QRegularExpression linePattern(QStringLiteral(R"(^\s*([A-Za-z0-9_-]+)\s+\[[0-9]+\]\s+([^
]+?)(?:\s+OPTIONAL)?\s*(?:,)?$)") , QRegularExpression::MultilineOption);
	QList<TcapParameterOption> values;
	auto it = linePattern.globalMatch(block);
	while (it.hasNext()) {
		const auto match = it.next();
		QString typeName = match.captured(2).trimmed();
		typeName.remove(QStringLiteral("{bound}"));
		values.append({match.captured(1).trimmed(), typeName.trimmed()});
	}
	return values;
}

const asn1_param kTcapRootParam{
	.name = const_cast<char *>("tcap"),
	.tag = ASN1_TYPE_ANY,
	.flags = 0,
	.bit = 0,
	.offset = 0,
	.type_desc = &tcap_tcmessage_type_desc,
	.register_val = false,
	.register_data = nullptr,
};

void assignOctetString(asn1_OCTET_STRING_t &target, const QByteArray &value)
{
	target.slice.data = reinterpret_cast<const uint8_t *>(value.constData());
	target.slice.size = static_cast<size_t>(value.size());
	target.internal = nullptr;
}
}

TcapEditorWidget::TcapEditorWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::TcapEditorWidget>())
{
	ui_->setupUi(this);
	if (ui_->treeParameters) {
		ui_->treeParameters->setColumnWidth(0, 180);
		ui_->treeParameters->setColumnWidth(1, 120);
		ui_->treeParameters->setColumnWidth(2, 120);
	}
	populateGeneratedDropdowns();
	wireSignals();
	previewStructure();
}

TcapEditorWidget::~TcapEditorWidget() = default;

QByteArray TcapEditorWidget::currentTemplateBuffer() const
{
	return buildTemplateJson();
}

QByteArray TcapEditorWidget::loadedTemplateBuffer() const
{
	return loadedTemplateBuffer_;
}

QString TcapEditorWidget::currentTemplatePath() const
{
	return selectedTemplatePath_;
}

bool TcapEditorWidget::saveTemplate()
{
	if (selectedTemplatePath_.isEmpty()) {
		return false;
	}
	return saveTemplateAs(selectedTemplatePath_);
}

bool TcapEditorWidget::saveTemplateAs(const QString &filePath)
{
	if (filePath.isEmpty()) {
		return false;
	}
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		return false;
	}
	const QByteArray buffer = buildTemplateJson();
	if (file.write(buffer) != buffer.size()) {
		return false;
	}
	selectedTemplatePath_ = filePath;
	loadedTemplateBuffer_ = buffer;
	return true;
}

void TcapEditorWidget::openTemplateFileDialog()
{
	const QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("Open TCAP Template"), selectedTemplatePath_, QStringLiteral("JSON Templates (*.json);;All Files (*)"));
	if (filePath.isEmpty()) {
		return;
	}
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return;
	}
	selectedTemplatePath_ = filePath;
	loadedTemplateBuffer_ = file.readAll();
	loadTemplateJson(loadedTemplateBuffer_);
	currentBuffer_ = encodeCurrentMessage();
	if (onPacketChanged_) {
		onPacketChanged_(currentBuffer_);
	}
}

void TcapEditorWidget::setOnPacketChanged(const std::function<void(const QByteArray &)> &onPacketChanged)
{
	onPacketChanged_ = onPacketChanged;
}

void TcapEditorWidget::setTemplateBuffer(const QByteArray &buffer)
{
	currentBuffer_ = buffer;
	if (!buffer.isEmpty() && ui_->plainSmsPayload) {
		ui_->plainSmsPayload->setPlainText(QString::fromLatin1(buffer.toHex(' ').toUpper()));
	}
}
void TcapEditorWidget::wireSignals()
{
	if (ui_->btnPreviewTcap) {
		connect(ui_->btnPreviewTcap, &QPushButton::clicked, this, [this]() { previewStructure(); });
	}
	if (ui_->btnPreviewHexTcap) {
		connect(ui_->btnPreviewHexTcap, &QPushButton::clicked, this, [this]() { previewHex(); });
	}
	if (ui_->btnGenerateTcap) {
		connect(ui_->btnGenerateTcap, &QPushButton::clicked, this, [this]() { previewHex(); });
	}
	if (ui_->btnValidateTcap) {
		connect(ui_->btnValidateTcap, &QPushButton::clicked, this, [this]() { validateInput(); });
	}
	if (ui_->comboAppFamily) {
		connect(ui_->comboAppFamily, &QComboBox::currentIndexChanged, this, [this](int) { refreshOperationOptions(); });
	}
	if (ui_->btnLoadTemplate) {
		connect(ui_->btnLoadTemplate, &QPushButton::clicked, this, [this]() { openTemplateFileDialog(); });
	}
	if (ui_->btnAddParam) {
		connect(ui_->btnAddParam, &QPushButton::clicked, this, [this]() { addParameter(false); });
	}
	if (ui_->btnAddChildParam) {
		connect(ui_->btnAddChildParam, &QPushButton::clicked, this, [this]() { addParameter(true); });
	}
	if (ui_->btnMoveParamUp) {
		connect(ui_->btnMoveParamUp, &QPushButton::clicked, this, [this]() { moveCurrentParameter(-1); });
	}
	if (ui_->btnMoveParamDown) {
		connect(ui_->btnMoveParamDown, &QPushButton::clicked, this, [this]() { moveCurrentParameter(1); });
	}
	if (ui_->btnRemoveParam && ui_->treeParameters) {
		connect(ui_->btnRemoveParam, &QPushButton::clicked, this, [this]() {
			delete ui_->treeParameters->currentItem();
		});
	}
}

void TcapEditorWidget::previewStructure()
{
	if (ui_->plainSmsPayload) {
		if (!currentBuffer_.isEmpty()) {
			const QString decoded = decodeCurrentBuffer();
			if (!decoded.isEmpty()) {
				ui_->plainSmsPayload->setPlainText(decoded);
				return;
			}
		}
		ui_->plainSmsPayload->setPlainText(buildStructureText());
	}
}

void TcapEditorWidget::previewHex()
{
	QString errorMessage;
	currentBuffer_ = encodeCurrentMessage(&errorMessage);
	if (currentBuffer_.isEmpty()) {
		currentBuffer_ = buildFallbackTcapBytes();
	}
	if (ui_->plainSmsPayload) {
		if (!errorMessage.isEmpty()) {
			ui_->plainSmsPayload->setPlainText(errorMessage + QStringLiteral("\n") + QString::fromLatin1(currentBuffer_.toHex(' ').toUpper()));
		} else {
			ui_->plainSmsPayload->setPlainText(QString::fromLatin1(currentBuffer_.toHex(' ').toUpper()));
		}
	}
	if (onPacketChanged_) {
		onPacketChanged_(currentBuffer_);
	}
}

void TcapEditorWidget::validateInput()
{
	const QByteArray otid = parseHexField(ui_->editOtid ? ui_->editOtid->text() : QString());
	const QByteArray dtid = parseHexField(ui_->editDtid ? ui_->editDtid->text() : QString());
	QStringList issues;
	const QString messageType = ui_->comboTcapMessageType ? ui_->comboTcapMessageType->currentText() : QString();
	if (messageType == QStringLiteral("Begin") && otid.isEmpty()) {
		issues << QStringLiteral("OTID is required for Begin.");
	}
	if ((messageType == QStringLiteral("Continue")) || (messageType == QStringLiteral("End")) || (messageType == QStringLiteral("Abort"))) {
		if (dtid.isEmpty()) {
			issues << QStringLiteral("DTID is required for this message type.");
		}
	}
	if (ui_->plainSmsPayload) {
		ui_->plainSmsPayload->setPlainText(issues.isEmpty() ? QStringLiteral("Validation passed.") : issues.join('\n'));
	}
}

QByteArray TcapEditorWidget::encodeCurrentMessage(QString *errorMessage) const
{
	tcap_tcmessage_t message{};
	QByteArray otid = parseHexField(ui_->editOtid ? ui_->editOtid->text() : QString());
	QByteArray dtid = parseHexField(ui_->editDtid ? ui_->editDtid->text() : QString());
	const QString messageType = ui_->comboTcapMessageType ? ui_->comboTcapMessageType->currentText() : QString();

	if (messageType == QStringLiteral("Begin")) {
		auto *begin = new tcap_begin_t{};
		begin->seen_mask = tcap_begin_MANDATORY_MASK;
		assignOctetString(begin->otid, otid);
		message.choice = tcap_tcmessage_begin;
		message.u.begin = begin;
	} else if (messageType == QStringLiteral("Continue")) {
		auto *cont = new tcap_continue_t{};
		cont->seen_mask = tcap_continue_MANDATORY_MASK;
		assignOctetString(cont->otid, otid);
		assignOctetString(cont->dtid, dtid);
		message.choice = tcap_tcmessage_continue;
		message.u.continue_ = cont;
	} else if (messageType == QStringLiteral("End")) {
		auto *end = new tcap_end_t{};
		end->seen_mask = tcap_end_MANDATORY_MASK;
		assignOctetString(end->dtid, dtid);
		message.choice = tcap_tcmessage_end;
		message.u.end = end;
	} else if (messageType == QStringLiteral("Abort")) {
		auto *abortMessage = new tcap_abort_t{};
		abortMessage->seen_mask = tcap_abort_MANDATORY_MASK;
		assignOctetString(abortMessage->dtid, dtid);
		message.choice = tcap_tcmessage_abort;
		message.u.abort = abortMessage;
	} else {
		auto *uni = new tcap_unidirectional_t{};
		uni->seen_mask = tcap_unidirectional_MANDATORY_MASK;
		message.choice = tcap_tcmessage_unidirectional;
		message.u.unidirectional = uni;
	}

	int encodedSize = asn1_encode_ber(&kTcapRootParam, &message, nullptr, 0);
	QByteArray encoded;
	if (encodedSize > 0) {
		encoded.resize(encodedSize);
		encodedSize = asn1_encode_ber(&kTcapRootParam, &message, reinterpret_cast<uint8_t *>(encoded.data()), static_cast<size_t>(encoded.size()));
		if (encodedSize < 0) {
			encoded.clear();
		}
	}
	if (encoded.isEmpty() && errorMessage) {
		*errorMessage = QStringLiteral("Generated TCAP BER encode failed. Using fallback preview.");
	}
	if (message.choice == tcap_tcmessage_begin) {
		delete message.u.begin;
	} else if (message.choice == tcap_tcmessage_continue) {
		delete message.u.continue_;
	} else if (message.choice == tcap_tcmessage_end) {
		delete message.u.end;
	} else if (message.choice == tcap_tcmessage_abort) {
		delete message.u.abort;
	} else if (message.choice == tcap_tcmessage_unidirectional) {
		delete message.u.unidirectional;
	}
	return encoded;
}

QString TcapEditorWidget::decodeCurrentBuffer() const
{
	if (currentBuffer_.isEmpty()) {
		return {};
	}
	tcap_tcmessage_t message{};
	if (asn1_decode_ber(&kTcapRootParam, reinterpret_cast<const uint8_t *>(currentBuffer_.constData()), static_cast<size_t>(currentBuffer_.size()), &message, nullptr) != 0) {
		return {};
	}
	char jsonBuffer[4096] = {};
	const size_t printed = asn1_print_json(&kTcapRootParam, &message, jsonBuffer, sizeof(jsonBuffer));
	asn1_free(&kTcapRootParam, &message);
	if (!printed) {
		return {};
	}
	return QString::fromLatin1(jsonBuffer, static_cast<int>(printed));
}

void TcapEditorWidget::populateGeneratedDropdowns()
{
	if (ui_->comboAppFamily) {
		ui_->comboAppFamily->clear();
		ui_->comboAppFamily->addItem(QStringLiteral("MAP"));
		ui_->comboAppFamily->addItem(QStringLiteral("CAP"));
	}
	if (ui_->comboService) {
		ui_->comboService->clear();
		ui_->comboService->addItem(QStringLiteral("Generated"));
	}
	refreshOperationOptions();
}

void TcapEditorWidget::refreshOperationOptions()
{
	if (!ui_->comboOperation || !ui_->comboAppFamily) {
		return;
	}
	ui_->comboOperation->clear();
	const QString family = ui_->comboAppFamily->currentText();
	if (family == QStringLiteral("MAP")) {
		const QStringList operations = loadOperationNames(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/gsmmap.c"), QRegularExpression(QStringLiteral("\\[(\\d+)\\] = \"([^\"]+)\"")));
		QStringList filtered;
		for (const QString &operation : operations) {
			if (!filtered.contains(operation)) {
				filtered.append(operation);
			}
		}
		ui_->comboOperation->addItems(filtered);
		return;
	}
	QStringList operations = loadOperationNames(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/cap-gprs-ssf-gsm-scf-ops-args.h"), QRegularExpression(QStringLiteral("^([A-Za-z0-9]+) \\{PARAMETERS-BOUND"), QRegularExpression::MultilineOption));
	operations.append(loadOperationNames(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/cap-gsm-ssf-gsm-scf-ops-args.h"), QRegularExpression(QStringLiteral("^([A-Za-z0-9]+) \\{PARAMETERS-BOUND"), QRegularExpression::MultilineOption)));
	operations.append(loadOperationNames(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/cap-sms-ops-args.h"), QRegularExpression(QStringLiteral("^([A-Za-z0-9]+) \\{PARAMETERS-BOUND"), QRegularExpression::MultilineOption)));
	operations.removeDuplicates();
	ui_->comboOperation->addItems(operations);
}

QStringList TcapEditorWidget::parameterOptionsForCurrentOperation() const
{
	return parameterOptionsForOperation(ui_->comboAppFamily ? ui_->comboAppFamily->currentText() : QString(), ui_->comboOperation ? ui_->comboOperation->currentText() : QString());
}

QList<TcapParameterOption> TcapEditorWidget::parameterDetailsForCurrentOperation() const
{
	return parameterDetailsForOperation(ui_->comboAppFamily ? ui_->comboAppFamily->currentText() : QString(), ui_->comboOperation ? ui_->comboOperation->currentText() : QString());
}

QStringList TcapEditorWidget::parameterOptionsForOperation(const QString &family, const QString &operation) const
{
	const QList<TcapParameterOption> details = parameterDetailsForOperation(family, operation);
	QStringList values;
	for (const TcapParameterOption &detail : details) {
		values.append(detail.name);
	}
	return values;
}

QList<TcapParameterOption> TcapEditorWidget::parameterDetailsForOperation(const QString &family, const QString &operation) const
{
	if (family == QStringLiteral("MAP")) {
		if (operation == QStringLiteral("updateLocation")) {
			return loadStructParameterDetails(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/map-ms-data-types.h"), QStringLiteral("map_update_location_arg"));
		}
		if (operation == QStringLiteral("sendAuthenticationInfo")) {
			return loadStructParameterDetails(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/map-ms-data-types.h"), QStringLiteral("map_send_authentication_info_arg"));
		}
		if (operation == QStringLiteral("insertSubscriberData")) {
			return loadStructParameterDetails(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/map-ms-data-types.h"), QStringLiteral("map_insert_subscriber_data_arg"));
		}
		if (operation == QStringLiteral("mo-forwardSM")) {
			return loadStructParameterDetails(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/map-sm-data-types.h"), QStringLiteral("map_mo_forward_sm_arg"));
		}
		if (operation == QStringLiteral("mt-forwardSM")) {
			return loadStructParameterDetails(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/map-sm-data-types.h"), QStringLiteral("map_mt_forward_sm_arg"));
		}
		if (operation == QStringLiteral("sendRoutingInfoForSM")) {
			return loadStructParameterDetails(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/map-sm-data-types.h"), QStringLiteral("map_routing_info_for_sm_arg"));
		}
		return {};
	}
	if (operation == QStringLiteral("initialDPGPRS")) {
		return loadSequenceParameterDetails(QStringLiteral(BOAR_SOURCE_DIR "/src/core/generated/cap-gprs-ssf-gsm-scf-ops-args.h"), QStringLiteral("InitialDPGPRSArg"));
	}
	return {};
}

void TcapEditorWidget::addParameter(bool asChild)
{
	if (!ui_->treeParameters) {
		return;
	}
	AddTcapParameterDialog dialog(this);
	const QList<TcapParameterOption> details = parameterDetailsForCurrentOperation();
	QStringList options;
	QHash<QString, QString> typeNames;
	for (const TcapParameterOption &detail : details) {
		options.append(detail.name);
		typeNames.insert(detail.name, detail.typeName);
	}
	dialog.setParameterDetails(options.isEmpty() ? QStringList{QStringLiteral("parameter")} : options, typeNames);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}
	auto *item = new QTreeWidgetItem(QStringList{dialog.selectedParameter(), QStringLiteral("ASN.1"), QStringLiteral("generated"), dialog.parameterValue()});
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	QTreeWidgetItem *current = ui_->treeParameters->currentItem();
	if (asChild && current) {
		current->addChild(item);
		current->setExpanded(true);
	} else if (current && current->parent()) {
		current->parent()->insertChild(current->parent()->indexOfChild(current) + 1, item);
	} else if (current) {
		ui_->treeParameters->insertTopLevelItem(ui_->treeParameters->indexOfTopLevelItem(current) + 1, item);
	} else {
		ui_->treeParameters->addTopLevelItem(item);
	}
	ui_->treeParameters->setCurrentItem(item);
}

void TcapEditorWidget::moveCurrentParameter(int delta)
{
	if (!ui_->treeParameters) {
		return;
	}
	QTreeWidgetItem *current = ui_->treeParameters->currentItem();
	if (!current) {
		return;
	}
	QTreeWidgetItem *parent = current->parent();
	if (parent) {
		const int index = parent->indexOfChild(current);
		const int nextIndex = index + delta;
		if (nextIndex < 0 || nextIndex >= parent->childCount()) {
			return;
		}
		parent->takeChild(index);
		parent->insertChild(nextIndex, current);
		ui_->treeParameters->setCurrentItem(current);
		return;
	}
	const int index = ui_->treeParameters->indexOfTopLevelItem(current);
	const int nextIndex = index + delta;
	if (nextIndex < 0 || nextIndex >= ui_->treeParameters->topLevelItemCount()) {
		return;
	}
	ui_->treeParameters->takeTopLevelItem(index);
	ui_->treeParameters->insertTopLevelItem(nextIndex, current);
	ui_->treeParameters->setCurrentItem(current);
}

QByteArray TcapEditorWidget::buildTemplateJson() const
{
	QJsonObject transaction;
	transaction.insert(QStringLiteral("messageType"), ui_->comboTcapMessageType ? ui_->comboTcapMessageType->currentText() : QString());
	transaction.insert(QStringLiteral("otid"), ui_->editOtid ? ui_->editOtid->text().trimmed() : QString());
	transaction.insert(QStringLiteral("dtid"), ui_->editDtid ? ui_->editDtid->text().trimmed() : QString());
	transaction.insert(QStringLiteral("applicationFamily"), ui_->comboAppFamily ? ui_->comboAppFamily->currentText() : QString());
	transaction.insert(QStringLiteral("service"), ui_->comboService ? ui_->comboService->currentText() : QString());
	transaction.insert(QStringLiteral("operation"), ui_->comboOperation ? ui_->comboOperation->currentText() : QString());
	transaction.insert(QStringLiteral("operationCode"), ui_->editOperationCode ? ui_->editOperationCode->text().trimmed() : QString());
	QJsonArray parameters;
	if (ui_->treeParameters) {
		for (int i = 0; i < ui_->treeParameters->topLevelItemCount(); ++i) {
			QTreeWidgetItem *item = ui_->treeParameters->topLevelItem(i);
			QJsonObject parameter;
			parameter.insert(QStringLiteral("name"), item->text(0));
			parameter.insert(QStringLiteral("type"), item->text(1));
			parameter.insert(QStringLiteral("tag"), item->text(2));
			parameter.insert(QStringLiteral("value"), item->text(3));
			parameters.append(parameter);
		}
	}
	transaction.insert(QStringLiteral("parameters"), parameters);
	QJsonObject root;
	root.insert(QStringLiteral("tcap"), transaction);
	return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

void TcapEditorWidget::loadTemplateJson(const QByteArray &buffer)
{
	const QJsonObject root = QJsonDocument::fromJson(buffer).object().value(QStringLiteral("tcap")).toObject();
	if (ui_->comboTcapMessageType) {
		ui_->comboTcapMessageType->setCurrentText(root.value(QStringLiteral("messageType")).toString());
	}
	if (ui_->editOtid) {
		ui_->editOtid->setText(root.value(QStringLiteral("otid")).toString());
	}
	if (ui_->editDtid) {
		ui_->editDtid->setText(root.value(QStringLiteral("dtid")).toString());
	}
	if (ui_->comboAppFamily) {
		ui_->comboAppFamily->setCurrentText(root.value(QStringLiteral("applicationFamily")).toString());
	}
	refreshOperationOptions();
	if (ui_->comboService) {
		ui_->comboService->setCurrentText(root.value(QStringLiteral("service")).toString());
	}
	if (ui_->comboOperation) {
		ui_->comboOperation->setCurrentText(root.value(QStringLiteral("operation")).toString());
	}
	if (ui_->editOperationCode) {
		ui_->editOperationCode->setText(root.value(QStringLiteral("operationCode")).toString());
	}
	if (ui_->treeParameters) {
		ui_->treeParameters->clear();
		const QJsonArray parameters = root.value(QStringLiteral("parameters")).toArray();
		for (const QJsonValue &value : parameters) {
			const QJsonObject parameter = value.toObject();
			auto *item = new QTreeWidgetItem(QStringList{parameter.value(QStringLiteral("name")).toString(), parameter.value(QStringLiteral("type")).toString(), parameter.value(QStringLiteral("tag")).toString(), parameter.value(QStringLiteral("value")).toString()});
			item->setFlags(item->flags() | Qt::ItemIsEditable);
			ui_->treeParameters->addTopLevelItem(item);
		}
	}
}

QByteArray TcapEditorWidget::buildFallbackTcapBytes() const
{
	const QString messageType = ui_->comboTcapMessageType ? ui_->comboTcapMessageType->currentText() : QString();
	const QByteArray otid = parseHexField(ui_->editOtid ? ui_->editOtid->text() : QString());
	const QByteArray dtid = parseHexField(ui_->editDtid ? ui_->editDtid->text() : QString());
	const QByteArray opCode = parseHexField(ui_->editOperationCode ? ui_->editOperationCode->text() : QString());
	QByteArray buffer;
	if (messageType == QStringLiteral("Begin")) {
		buffer.append(char(0x62));
		QByteArray body;
		if (!otid.isEmpty()) {
			body.append(char(0x48));
			body.append(char(otid.size()));
			body.append(otid);
		}
		if (!opCode.isEmpty()) {
			body.append(char(0x6C));
			body.append(char(opCode.size()));
			body.append(opCode);
		}
		buffer.append(char(body.size()));
		buffer.append(body);
	} else if (messageType == QStringLiteral("Continue")) {
		buffer.append(char(0x65));
		QByteArray body;
		if (!otid.isEmpty()) {
			body.append(char(0x48));
			body.append(char(otid.size()));
			body.append(otid);
		}
		if (!dtid.isEmpty()) {
			body.append(char(0x49));
			body.append(char(dtid.size()));
			body.append(dtid);
		}
		buffer.append(char(body.size()));
		buffer.append(body);
	} else if (messageType == QStringLiteral("End")) {
		buffer.append(char(0x64));
		QByteArray body;
		if (!dtid.isEmpty()) {
			body.append(char(0x49));
			body.append(char(dtid.size()));
			body.append(dtid);
		}
		buffer.append(char(body.size()));
		buffer.append(body);
	} else if (messageType == QStringLiteral("Abort")) {
		buffer.append(char(0x67));
		QByteArray body;
		if (!dtid.isEmpty()) {
			body.append(char(0x49));
			body.append(char(dtid.size()));
			body.append(dtid);
		}
		buffer.append(char(body.size()));
		buffer.append(body);
	} else {
		buffer.append(char(0x61));
		buffer.append(char(0x00));
	}
	return buffer;
}

QString TcapEditorWidget::buildStructureText() const
{
	QStringList lines;
	lines << QStringLiteral("TCAP Message");
	if (ui_->comboTcapMessageType) {
		lines << QStringLiteral("Type: %1").arg(ui_->comboTcapMessageType->currentText());
	}
	if (ui_->editOtid && !ui_->editOtid->text().trimmed().isEmpty()) {
		lines << QStringLiteral("OTID: %1").arg(ui_->editOtid->text().trimmed());
	}
	if (ui_->editDtid && !ui_->editDtid->text().trimmed().isEmpty()) {
		lines << QStringLiteral("DTID: %1").arg(ui_->editDtid->text().trimmed());
	}
	if (ui_->comboOperation) {
		lines << QStringLiteral("Operation: %1").arg(ui_->comboOperation->currentText());
	}
	if (ui_->editOperationCode && !ui_->editOperationCode->text().trimmed().isEmpty()) {
		lines << QStringLiteral("Operation Code: %1").arg(ui_->editOperationCode->text().trimmed());
	}
	return lines.join('\n');
}

QByteArray TcapEditorWidget::parseHexField(const QString &text)
{
	QByteArray compact = text.toLatin1();
	compact.replace(" ", "");
	compact.replace("0x", "");
	compact.replace("0X", "");
	return QByteArray::fromHex(compact);
}
