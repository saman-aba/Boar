#include "dashboardmodulewidget.h"

#include "../dialogs/packetexportdialog.h"

#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "scenariobuilderwidget.h"
#include "ui_DashboardModuleWidget.h"

namespace
{
QString packetLabel(const ForgedPacketRecord &packet)
{
	return packet.name.isEmpty() ? QStringLiteral("Unnamed Packet") : packet.name;
}
}

DashboardModuleWidget::DashboardModuleWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(std::make_unique<Ui::DashboardModuleWidget>())
	, scenarioBuilderWidget_(std::make_unique<ScenarioBuilderWidget>(this))
{
	ui_->setupUi(this);
	if (ui_->dashboardTabs) {
		ui_->dashboardTabs->addTab(scenarioBuilderWidget_.get(), QStringLiteral("Scenario Builder"));
	}
	if (ui_->treeRecentSessions) {
		ui_->treeRecentSessions->setColumnCount(6);
		ui_->treeRecentSessions->setHeaderLabels({
				QStringLiteral("Packet"),
				QStringLiteral("Protocol"),
				QStringLiteral("Transport"),
				QStringLiteral("Source"),
				QStringLiteral("Destination"),
				QStringLiteral("Status")
		});
		connect(ui_->treeRecentSessions, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *, int) {
			const auto packet = currentPacket();
			if (!packet.id.isEmpty() && onOpenPacket_) {
				onOpenPacket_(packet);
			}
		});
	}
	if (auto *button = findChild<QPushButton *>(QStringLiteral("btnOpenPacketGen"))) {
		connect(button, &QPushButton::clicked, this, [this]() {
			if (onOpenPacketGenerator_) {
				onOpenPacketGenerator_();
			}
		});
	}
	if (auto *button = findChild<QPushButton *>(QStringLiteral("btnOpenNodeEmu"))) {
		connect(button, &QPushButton::clicked, this, [this]() {
			if (onOpenPacket_) {
				const auto packet = currentPacket();
				if (!packet.id.isEmpty()) {
					onOpenPacket_(packet);
				}
			}
		});
	}
	if (auto *button = findChild<QPushButton *>(QStringLiteral("btnOpenDb"))) {
		connect(button, &QPushButton::clicked, this, [this]() {
			appendLog(QStringLiteral("Database launch is not implemented in this build"));
		});
	}
	if (auto *button = findChild<QPushButton *>(QStringLiteral("btnOpenMessaging"))) {
		connect(button, &QPushButton::clicked, this, [this]() {
			appendLog(QStringLiteral("Messaging launch is not implemented in this build"));
		});
	}
	if (auto *button = findChild<QPushButton *>(QStringLiteral("btnPacketAdd"))) {
		connect(button, &QPushButton::clicked, this, [this]() {
			if (onOpenPacketGenerator_) {
				onOpenPacketGenerator_();
			}
		});
	}
	if (auto *button = findChild<QPushButton *>(QStringLiteral("btnPacketRemove"))) {
		connect(button, &QPushButton::clicked, this, [this]() {
			const auto packet = currentPacket();
			if (packet.id.isEmpty() || !onRemovePacket_) {
				return;
			}

			const auto answer = QMessageBox::question(this,
					QStringLiteral("Remove Packet"),
					QStringLiteral("Remove selected packet from dashboard?"),
					QMessageBox::Yes | QMessageBox::No,
					QMessageBox::No);

			if (answer != QMessageBox::Yes) {
				return;
			}
			onRemovePacket_(packet.id);
		});
	}
	if (auto *button = findChild<QPushButton *>(QStringLiteral("btnPacketEdit"))) {
		connect(button, &QPushButton::clicked, this, [this]() {
			const auto packet = currentPacket();
			if (packet.id.isEmpty() || !onEditPacket_) {
				return;
			}
			onEditPacket_(packet.id);
		});
	}
	if (auto *button = findChild<QPushButton *>(QStringLiteral("btnPacketExport"))) {
		connect(button, &QPushButton::clicked, this, [this]() {
			requestExportCurrentPacket();
		});
	}
	if (auto *button = findChild<QPushButton *>(QStringLiteral("btnTransmitSelected"))) {
		connect(button, &QPushButton::clicked, this, [this]() { requestTransmitCurrentPacket(); });
	}
}

DashboardModuleWidget::~DashboardModuleWidget() = default;

void DashboardModuleWidget::setPackets(const QList<ForgedPacketRecord> &packets)
{
	packets_ = packets;
	refreshPacketTree();
}

void DashboardModuleWidget::addPacket(const ForgedPacketRecord &packet)
{
	packets_.prepend(packet);
	refreshPacketTree();
}

void DashboardModuleWidget::updatePacket(const ForgedPacketRecord &packet)
{
	for (auto &existingPacket : packets_) {
		if (existingPacket.id == packet.id) {
			existingPacket = packet;
			refreshPacketTree();
			setPacketsModelSelection(packet.id);
			return;
		}
	}
	addPacket(packet);
}

void DashboardModuleWidget::setOnOpenPacketGenerator(const std::function<void()> &callback)
{
	onOpenPacketGenerator_ = callback;
}

void DashboardModuleWidget::setOnOpenPacket(const std::function<void(const ForgedPacketRecord &)> &callback)
{
	onOpenPacket_ = callback;
}

void DashboardModuleWidget::setOnTransmitPacket(const std::function<void(const ForgedPacketRecord &, const QString &)> &callback)
{
	onTransmitPacket_ = callback;
}

void DashboardModuleWidget::setOnRemovePacket(const std::function<void(const QString &)> &callback)
{
	onRemovePacket_ = callback;
}

void DashboardModuleWidget::setOnEditPacket(const std::function<void(const QString &)> &callback)
{
	onEditPacket_ = callback;
}

void DashboardModuleWidget::setOnExportPacket(const std::function<void(const ForgedPacketRecord &)> &callback)
{
	onExportPacket_ = callback;
}

void DashboardModuleWidget::removePacket(const QString &packetId)
{
	for (auto it = packets_.begin(); it != packets_.end(); ++it) {
		if (it->id == packetId) {
			packets_.erase(it);
			break;
		}
	}
	refreshPacketTree();
}

void DashboardModuleWidget::appendLog(const QString &message)
{
	if (auto *logConsole = window()->findChild<QPlainTextEdit *>(QStringLiteral("logConsole"))) {
		logConsole->appendPlainText(message);
	}
}

void DashboardModuleWidget::refreshPacketTree()
{
	if (!ui_->treeRecentSessions) {
		return;
	}
	ui_->treeRecentSessions->clear();
	for (const auto &packet : packets_) {
		auto *item = new QTreeWidgetItem(ui_->treeRecentSessions);
		item->setText(0, packetLabel(packet));
		item->setText(1, packet.protocol);

		item->setText(2, packet.transportProtocol.isEmpty() ?
				QStringLiteral("Raw") :
				packet.transportProtocol);

		item->setText(3, packet.sourceIp.isEmpty() ?
				packet.sourceMac :
				packet.sourceIp);

		item->setText(4, packet.destinationIp.isEmpty() ?
				packet.destinationMac :
				packet.destinationIp);

		item->setText(5, packet.summary.isEmpty() ?
				QStringLiteral("Ready") :
				packet.summary);

		item->setData(0, Qt::UserRole, packet.id);
	}
	setPacketsModelSelection(packets_.isEmpty() ?
			QString() :
			packets_.first().id);
}

void DashboardModuleWidget::setPacketsModelSelection(const QString &packetId)
{
	if (!ui_->treeRecentSessions) {
		return;
	}
	auto *selectedItem = ui_->treeRecentSessions->currentItem();
	if (selectedItem && selectedItem->data(0, Qt::UserRole).toString() == packetId) {
		return;
	}
	if (packetId.isEmpty()) {
		if (ui_->treeRecentSessions->topLevelItemCount() > 0) {
			ui_->treeRecentSessions->setCurrentItem(ui_->treeRecentSessions->topLevelItem(0));
		}
		return;
	}
	for (int row = 0; row < ui_->treeRecentSessions->topLevelItemCount(); ++row) {
		auto *item = ui_->treeRecentSessions->topLevelItem(row);
		if (item && item->data(0, Qt::UserRole).toString() == packetId) {
			ui_->treeRecentSessions->setCurrentItem(item);
			return;
		}
	}
}

void DashboardModuleWidget::requestExportCurrentPacket()
{
	const auto packet = currentPacket();
	if (packet.id.isEmpty()) {
		return;
	}
	PacketExportDialog dialog(this);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}
	const QString fileName = packet.exportBaseName.isEmpty() ?
			QStringLiteral("packet") :
			packet.exportBaseName;

	QString filePath;
	switch (dialog.exportType()) {
	case PacketExportDialog::ExportType::Pcap:
		filePath = QFileDialog::getSaveFileName(this,
				QStringLiteral("Export PCAP"),
				fileName + QStringLiteral(".pcap"),
				QStringLiteral("PCAP Files (*.pcap)"));
		break;
	case PacketExportDialog::ExportType::HexDump:
		filePath = QFileDialog::getSaveFileName(this,
				QStringLiteral("Export Hex Dump"),
				fileName + QStringLiteral(".hex"),
				QStringLiteral("Hex Files (*.hex);;Text Files (*.txt)"));
		break;
	case PacketExportDialog::ExportType::HexString:
		filePath = QFileDialog::getSaveFileName(this,
				QStringLiteral("Export Hex String"),
				fileName + QStringLiteral(".txt"),
				QStringLiteral("Text Files (*.txt)"));
		break;
	case PacketExportDialog::ExportType::Binary:
		filePath = QFileDialog::getSaveFileName(this,
				QStringLiteral("Export Binary"),
				fileName + QStringLiteral(".bin"),
				QStringLiteral("Binary Files (*.bin)"));
		break;
	}
	if (filePath.isEmpty()) {
		return;
	}
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return;
	}
	switch (dialog.exportType()) {
	case PacketExportDialog::ExportType::Pcap:
		if (packet.pcapBytes.isEmpty()) {
			file.write(packet.frameBytes);
		} else {
			file.write(packet.pcapBytes);
		}
		break;
	case PacketExportDialog::ExportType::HexDump:
		file.write(packet.payload.toHex(' ').toUpper());
		break;
	case PacketExportDialog::ExportType::HexString:
		file.write(packet.payload.toHex().toUpper());
		break;
	case PacketExportDialog::ExportType::Binary:
		file.write(packet.payload);
		break;
	}
	if (onExportPacket_) {
		onExportPacket_(packet);
	}
}

ForgedPacketRecord DashboardModuleWidget::currentPacket() const
{
	if (!ui_->treeRecentSessions || !ui_->treeRecentSessions->currentItem()) {
		return {};
	}
	const QString packetId = ui_->treeRecentSessions->currentItem()->data(0, Qt::UserRole).toString();
	for (const auto &packet : packets_) {
		if (packet.id == packetId) {
			return packet;
		}
	}
	return {};
}

void DashboardModuleWidget::requestTransmitCurrentPacket()
{
	const auto packet = currentPacket();
	if (packet.id.isEmpty() || !onTransmitPacket_) {
		return;
	}
	const QString interfaceName = QInputDialog::getText(this,
			QStringLiteral("Transmit Packet"),
			QStringLiteral("Interface / channel name"));

	if (interfaceName.isEmpty()) {
		return;
	}
	onTransmitPacket_(packet, interfaceName);
}
