#include "packeteditormodulewidget.h"

#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHeaderView>
#include <QMainWindow>
#include <QStatusBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <array>

#include "../dialogs/exportpcapdialog.h"
#include "diametereditorwidget.h"
#include "tcapeditorwidget.h"
#include "ui_PacketEditorModuleWidget.h"

PacketEditorModuleWidget::PacketEditorModuleWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::PacketEditorModuleWidget>())
	, diameterEditorWidget_(std::make_unique<DiameterEditorWidget>(this))
	, tcapEditorWidget_(std::make_unique<TcapEditorWidget>(this))
{
	ui_->setupUi(this);
	if (ui_->tableHexEditor) {
		QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
		font.setFixedPitch(true);
		font.setKerning(false);
		font.setPointSize(10);
		ui_->tableHexEditor->setFont(font);
		ui_->tableHexEditor->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
		ui_->tableHexEditor->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
		ui_->tableHexEditor->verticalHeader()->setDefaultSectionSize(20);
		ui_->tableHexEditor->horizontalHeader()->setDefaultSectionSize(24);
		ui_->tableHexEditor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		ui_->tableHexEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		ui_->tableHexEditor->setSelectionMode(QAbstractItemView::SingleSelection);
		ui_->tableHexEditor->setSelectionBehavior(QAbstractItemView::SelectItems);
		ui_->tableHexEditor->setAlternatingRowColors(false);
	}
	ui_->protocolStack->addWidget(diameterEditorWidget_.get());
	ui_->protocolStack->addWidget(new QWidget(this));
	ui_->protocolStack->addWidget(tcapEditorWidget_.get());
	if (ui_->splitterContent) {
		ui_->splitterContent->setChildrenCollapsible(false);
		ui_->splitterContent->setStretchFactor(0, 3);
		ui_->splitterContent->setStretchFactor(1, 1);
		ui_->splitterContent->setSizes({720, 240});
	}
	diameterEditorWidget_->setOnTemplateChanged([this](const QByteArray &buffer) {
		if (syncingHexView_) {
			return;
		}
		currentPacketBytes_ = buffer;
		syncHexEditorFromBuffer();
	});
	tcapEditorWidget_->setOnPacketChanged([this](const QByteArray &buffer) {
		if (syncingHexView_) {
			return;
		}
		currentPacketBytes_ = buffer;
		syncHexEditorFromBuffer();
	});
	connect(ui_->comboProtocolSelector, &QComboBox::currentIndexChanged, this, [this](int index) {
		ui_->protocolStack->setCurrentIndex(index);
	});
	if (ui_->btnLoadPcap) {
		connect(ui_->btnLoadPcap, &QPushButton::clicked, this, [this]() {
			loadHexFromFile();
			if (ui_->comboProtocolSelector && ui_->comboProtocolSelector->currentIndex() == 0) {
				diameterEditorWidget_->openTemplateFileDialog();
				const QByteArray loadedBuffer = diameterEditorWidget_->loadedTemplateBuffer();
				if (!loadedBuffer.isEmpty()) {
					currentPacketBytes_ = loadedBuffer;
					syncHexEditorFromBuffer();
				}
				return;
			}
			if (ui_->comboProtocolSelector && ui_->comboProtocolSelector->currentIndex() == 2) {
				tcapEditorWidget_->openTemplateFileDialog();
				const QByteArray loadedBuffer = tcapEditorWidget_->loadedTemplateBuffer();
				if (!loadedBuffer.isEmpty()) {
					currentPacketBytes_ = tcapEditorWidget_->encodeCurrentMessage();
					syncHexEditorFromBuffer();
				}
			}
		});
	}
	if (ui_->btnExportPcap) {
		connect(ui_->btnExportPcap, &QPushButton::clicked, this, [this]() {
			exportPcap();
		});
	}
	if (ui_->btnSaveCurrentTemplate) {
		connect(ui_->btnSaveCurrentTemplate, &QPushButton::clicked, this, [this]() {
			saveTemplate();
		});
	}
	if (ui_->btnSaveTemplate) {
		connect(ui_->btnSaveTemplate, &QPushButton::clicked, this, [this]() {
			saveTemplateAs();
		});
	}
	if (ui_->btnValidate) {
		connect(ui_->btnValidate, &QPushButton::clicked, this, [this]() {
			validateHexEditor();
		});
	}
	if (ui_->btnCloseTemplate) {
		connect(ui_->btnCloseTemplate, &QPushButton::clicked, this, [this]() {
			closeCurrentTemplate();
		});
	}
	if (ui_->tableHexEditor) {
		connect(ui_->tableHexEditor, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *) {
			if (syncingHexView_) {
				return;
			}
			currentPacketBytes_ = parseHexEditorText();
			applyHexEditorToActiveProtocol();
		});
	}
}

void PacketEditorModuleWidget::loadHexFromFile()
{
	currentPacketBytes_ = parseHexEditorText();
}

void PacketEditorModuleWidget::syncHexEditorFromBuffer()
{
	if (!ui_->tableHexEditor) {
		return;
	}
	syncingHexView_ = true;
	const QSignalBlocker blocker(ui_->tableHexEditor);
	populateHexTable(currentPacketBytes_);
	syncingHexView_ = false;
}

void PacketEditorModuleWidget::exportPcap()
{
	const QByteArray packetBytes = parseHexEditorText();
	if (packetBytes.isEmpty()) {
		return;
	}
	ExportPcapDialog dialog(this);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}
	const QString exportDirectory = dialog.exportDirectory();
	if (exportDirectory.isEmpty()) {
		return;
	}
	const QByteArray frameBytes = buildExportFrame(packetBytes, dialog);
	if (frameBytes.isEmpty()) {
		return;
	}
	QFile file(QDir(exportDirectory).filePath(QStringLiteral("packet.pcap")));
	if (!file.open(QIODevice::WriteOnly)) {
		return;
	}
	QByteArray pcapData;
	pcapData.append(QByteArray::fromHex("D4C3B2A1020004000000000000000000FFFF000001000000"));
	const quint32 capturedLength = static_cast<quint32>(frameBytes.size());
	auto appendLe32 = [&pcapData](quint32 value) {
		pcapData.append(static_cast<char>(value & 0xFF));
		pcapData.append(static_cast<char>((value >> 8) & 0xFF));
		pcapData.append(static_cast<char>((value >> 16) & 0xFF));
		pcapData.append(static_cast<char>((value >> 24) & 0xFF));
	};
	appendLe32(0);
	appendLe32(0);
	appendLe32(capturedLength);
	appendLe32(capturedLength);
	pcapData.append(frameBytes);
	file.write(pcapData);
}

QByteArray PacketEditorModuleWidget::buildExportFrame(const QByteArray &payload, const ExportPcapDialog &dialog) const
{
	auto appendBe16 = [](QByteArray &buffer, quint16 value) {
		buffer.append(static_cast<char>((value >> 8) & 0xFF));
		buffer.append(static_cast<char>(value & 0xFF));
	};
	auto appendBe32 = [](QByteArray &buffer, quint32 value) {
		buffer.append(static_cast<char>((value >> 24) & 0xFF));
		buffer.append(static_cast<char>((value >> 16) & 0xFF));
		buffer.append(static_cast<char>((value >> 8) & 0xFF));
		buffer.append(static_cast<char>(value & 0xFF));
	};
	QByteArray transportBytes = payload;
	const int transportIndex = dialog.transportProtocolIndex();
	if (transportIndex == 0) {
		QByteArray udpHeader;
		appendBe16(udpHeader, dialog.sourcePort());
		appendBe16(udpHeader, dialog.destinationPort());
		appendBe16(udpHeader, static_cast<quint16>(8 + payload.size()));
		appendBe16(udpHeader, 0);
		transportBytes = udpHeader + payload;
	} else if (transportIndex == 1) {
		QByteArray tcpHeader;
		appendBe16(tcpHeader, dialog.sourcePort());
		appendBe16(tcpHeader, dialog.destinationPort());
		appendBe32(tcpHeader, 0);
		appendBe32(tcpHeader, 0);
		tcpHeader.append(char(0x50));
		tcpHeader.append(char(0x18));
		appendBe16(tcpHeader, 65535);
		appendBe16(tcpHeader, 0);
		appendBe16(tcpHeader, 0);
		transportBytes = tcpHeader + payload;
	} else {
		QByteArray sctpBytes;
		appendBe16(sctpBytes, dialog.sourcePort());
		appendBe16(sctpBytes, dialog.destinationPort());
		appendBe32(sctpBytes, dialog.sctpVerificationTag());
		appendBe32(sctpBytes, 0);
		sctpBytes.append(char(0x00));
		sctpBytes.append(char(0x03));
		appendBe16(sctpBytes, static_cast<quint16>(16 + payload.size()));
		appendBe32(sctpBytes, 0);
		appendBe16(sctpBytes, 0);
		appendBe16(sctpBytes, 0);
		appendBe32(sctpBytes, dialog.sctpPayloadProtocolId());
		sctpBytes.append(payload);
		transportBytes = sctpBytes;
	}
	QByteArray ipBytes;
	ipBytes.append(char(0x45));
	ipBytes.append(char(0x00));
	appendBe16(ipBytes, static_cast<quint16>(20 + transportBytes.size()));
	appendBe16(ipBytes, 0);
	appendBe16(ipBytes, 0);
	ipBytes.append(char(dialog.ttl()));
	ipBytes.append(char(transportIndex == 0 ? 17 : (transportIndex == 1 ? 6 : 132)));
	appendBe16(ipBytes, 0);
	QString sourceAddress = dialog.sourceAddress();
	sourceAddress.replace('.', ' ');
	QString destinationAddress = dialog.destinationAddress();
	destinationAddress.replace('.', ' ');
	QByteArray sourceIp = parseHexBytes(sourceAddress, 4);
	QByteArray destinationIp = parseHexBytes(destinationAddress, 4);
	if (sourceIp.size() != 4 || destinationIp.size() != 4) {
		return {};
	}
	ipBytes.append(sourceIp);
	ipBytes.append(destinationIp);
	ipBytes.append(transportBytes);
	QByteArray frame;
	QByteArray dstMac = parseHexBytes(dialog.destinationMac(), 6);
	QByteArray srcMac = parseHexBytes(dialog.sourceMac(), 6);
	if (dstMac.size() != 6 || srcMac.size() != 6) {
		return {};
	}
	frame.append(dstMac);
	frame.append(srcMac);
	appendBe16(frame, 0x0800);
	frame.append(ipBytes);
	return frame;
}
PacketEditorModuleWidget::~PacketEditorModuleWidget() = default;

void PacketEditorModuleWidget::populateHexTable(const QByteArray &bytes)
{
	if (!ui_->tableHexEditor) {
		return;
	}
	const int rowCount = (bytes.size() + 7) / 8;
	ui_->tableHexEditor->clearContents();
	ui_->tableHexEditor->setRowCount(rowCount);
	for (int row = 0; row < rowCount; ++row) {
		ui_->tableHexEditor->setVerticalHeaderItem(row, new QTableWidgetItem(QStringLiteral("%1").arg(row * 8, 4, 16, QChar::fromLatin1('0')).toUpper()));
		for (int column = 0; column < 8; ++column) {
			const int index = row * 8 + column;
			auto *item = new QTableWidgetItem(index < bytes.size()
				? QStringLiteral("%1").arg(static_cast<unsigned char>(bytes.at(index)), 2, 16, QChar::fromLatin1('0')).toUpper()
				: QString());
			item->setTextAlignment(Qt::AlignCenter);
			ui_->tableHexEditor->setItem(row, column, item);
		}
	}
}

QByteArray PacketEditorModuleWidget::parseHexBytes(const QString &text, int expectedSize)
{
	QString normalized = text;
	normalized.replace(':', ' ');
	normalized.replace('-', ' ');
	QByteArray compact;
	const QStringList parts = normalized.split(' ', Qt::SkipEmptyParts);
	for (const QString &part : parts) {
		bool ok = false;
		const int value = part.toInt(&ok, 10);
		if (ok && expectedSize == 4) {
			compact.append(QByteArray::number(value, 16).rightJustified(2, '0'));
			continue;
		}
		compact.append(part.toLatin1());
	}
	QByteArray bytes = QByteArray::fromHex(compact);
	if (expectedSize >= 0 && bytes.size() != expectedSize) {
		return {};
	}
	return bytes;
}

QByteArray PacketEditorModuleWidget::parseHexEditorText() const
{
	if (!ui_->tableHexEditor) {
		return {};
	}
	QByteArray bytes;
	for (int row = 0; row < ui_->tableHexEditor->rowCount(); ++row) {
		for (int column = 0; column < ui_->tableHexEditor->columnCount(); ++column) {
			const auto *item = ui_->tableHexEditor->item(row, column);
			if (!item) {
				continue;
			}
			const QString text = item->text().trimmed().toUpper();
			if (text.isEmpty()) {
				continue;
			}
			const QByteArray cell = QByteArray::fromHex(text.toUtf8());
			if (cell.size() == 1) {
				bytes.append(cell);
			}
		}
	}
	return bytes;
}

void PacketEditorModuleWidget::validateHexEditor()
{
	currentPacketBytes_ = parseHexEditorText();
	syncHexEditorFromBuffer();
}

void PacketEditorModuleWidget::applyHexEditorToActiveProtocol()
{
	if (!ui_->comboProtocolSelector) {
		return;
	}
	if (ui_->comboProtocolSelector->currentIndex() == 0) {
		diameterEditorWidget_->setTemplateBuffer(currentPacketBytes_);
		return;
	}
	if (ui_->comboProtocolSelector->currentIndex() == 2) {
		tcapEditorWidget_->setTemplateBuffer(currentPacketBytes_);
	}
}

void PacketEditorModuleWidget::saveTemplate()
{
	if (!ui_->comboProtocolSelector) {
		return;
	}
	if (ui_->comboProtocolSelector->currentIndex() == 0) {
		if (diameterEditorWidget_->saveTemplate()) {
			currentPacketBytes_ = diameterEditorWidget_->currentTemplateBuffer();
			syncHexEditorFromBuffer();
			showStatusMessage(QStringLiteral("Template saved"));
			return;
		}
		showStatusMessage(QStringLiteral("No current template path. Use Save As Template."));
		return;
	}
	if (ui_->comboProtocolSelector->currentIndex() == 2) {
		if (tcapEditorWidget_->saveTemplate()) {
			currentPacketBytes_ = tcapEditorWidget_->encodeCurrentMessage();
			syncHexEditorFromBuffer();
			showStatusMessage(QStringLiteral("Template saved"));
			return;
		}
		showStatusMessage(QStringLiteral("No current template path. Use Save As Template."));
	}
}

void PacketEditorModuleWidget::saveTemplateAs()
{
	if (!ui_->comboProtocolSelector) {
		return;
	}
	const bool isDiameter = ui_->comboProtocolSelector->currentIndex() == 0;
	const bool isTcap = ui_->comboProtocolSelector->currentIndex() == 2;
	if (!isDiameter && !isTcap) {
		return;
	}
	const QString filePath = QFileDialog::getSaveFileName(this,
		isDiameter ? QStringLiteral("Save Diameter Template") : QStringLiteral("Save TCAP Template"),
		isDiameter ? diameterEditorWidget_->currentTemplatePath() : tcapEditorWidget_->currentTemplatePath(),
		QStringLiteral("JSON Templates (*.json);;All Files (*)"));
	if (filePath.isEmpty()) {
		showStatusMessage(QStringLiteral("Template name is required"));
		return;
	}
	if (QFileInfo(filePath).fileName().isEmpty()) {
		showStatusMessage(QStringLiteral("Template name is required"));
		return;
	}
	const bool saved = isDiameter ? diameterEditorWidget_->saveTemplateAs(filePath) : tcapEditorWidget_->saveTemplateAs(filePath);
	if (!saved) {
		showStatusMessage(QStringLiteral("Failed to save template"));
		return;
	}
	currentPacketBytes_ = isDiameter ? diameterEditorWidget_->currentTemplateBuffer() : tcapEditorWidget_->encodeCurrentMessage();
	syncHexEditorFromBuffer();
	showStatusMessage(QStringLiteral("Template saved"));
}

void PacketEditorModuleWidget::closeCurrentTemplate()
{
	currentPacketBytes_.clear();
	if (ui_->tableHexEditor) {
		const QSignalBlocker blocker(ui_->tableHexEditor);
		ui_->tableHexEditor->clearContents();
		ui_->tableHexEditor->setRowCount(0);
	}
	if (ui_->comboProtocolSelector && ui_->comboProtocolSelector->currentIndex() == 0) {
		diameterEditorWidget_->clearTemplate();
	}
	if (ui_->comboProtocolSelector && ui_->comboProtocolSelector->currentIndex() == 2) {
		tcapEditorWidget_->setTemplateBuffer({});
	}
}

void PacketEditorModuleWidget::showStatusMessage(const QString &message) const
{
	if (auto *mainWindow = qobject_cast<QMainWindow *>(this->window())) {
		if (mainWindow->statusBar()) {
			mainWindow->statusBar()->showMessage(message, 3000);
		}
	}
}
