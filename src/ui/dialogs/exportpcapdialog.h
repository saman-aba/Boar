#pragma once

#include <QDialog>

#include <QtGlobal>

#include <memory>

namespace Ui
{
class ExportPcapDialog;
}

class ExportPcapDialog : public QDialog
{
public:
	explicit ExportPcapDialog(QWidget *parent = nullptr);
	~ExportPcapDialog() override;

	QString exportDirectory() const;
	QString sourceMac() const;
	QString destinationMac() const;
	QString sourceAddress() const;
	QString destinationAddress() const;
	quint16 sourcePort() const;
	quint16 destinationPort() const;
	quint32 sctpPayloadProtocolId() const;
	quint32 sctpVerificationTag() const;
	int transportProtocolIndex() const;
	int ttl() const;
	bool isSctpSelected() const;

private:
	std::unique_ptr<Ui::ExportPcapDialog> ui_;
};
