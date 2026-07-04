#pragma once

#include <QDialog>

#include <QList>

class QAbstractButton;
class QComboBox;
class QPlainTextEdit;
class QDialogButtonBox;

class PacketExportDialog : public QDialog
{
public:
	explicit PacketExportDialog(QWidget *parent = nullptr);
	~PacketExportDialog() override;

	enum class ExportType
	{
		Pcap = 0,
		HexDump = 1,
		HexString = 2,
		Binary = 3,
	};

	[[nodiscard]] ExportType exportType() const;
private:
	QComboBox *exportTypeCombo_ = nullptr;
	QPlainTextEdit *descriptionEdit_ = nullptr;
	QDialogButtonBox *buttonBox_ = nullptr;
};
