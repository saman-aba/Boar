#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QSpinBox;

class SccpParametersDialog : public QDialog
{
public:
	explicit SccpParametersDialog(QWidget *parent = nullptr);
	~SccpParametersDialog() override;

	[[nodiscard]] QString sourceMac() const;
	[[nodiscard]] QString destinationMac() const;
	[[nodiscard]] QString sourceAddress() const;
	[[nodiscard]] QString destinationAddress() const;
	[[nodiscard]] quint16 sourcePort() const;
	[[nodiscard]] quint16 destinationPort() const;
	[[nodiscard]] quint32 verificationTag() const;
	[[nodiscard]] quint32 payloadProtocolId() const;
	[[nodiscard]] int ttl() const;
	[[nodiscard]] QString sctpChunkType() const;
	[[nodiscard]] QString sccpMessageType() const;
	[[nodiscard]] bool includeM3ua() const;
	[[nodiscard]] QString packetName() const;

private:
	QLineEdit *packetNameEdit_ = nullptr;
	QLineEdit *sourceMacEdit_ = nullptr;
	QLineEdit *destinationMacEdit_ = nullptr;
	QLineEdit *sourceAddressEdit_ = nullptr;
	QLineEdit *destinationAddressEdit_ = nullptr;
	QSpinBox *sourcePortSpin_ = nullptr;
	QSpinBox *destinationPortSpin_ = nullptr;
	QLineEdit *verificationTagEdit_ = nullptr;
	QLineEdit *payloadProtocolIdEdit_ = nullptr;
	QSpinBox *ttlSpin_ = nullptr;
	QComboBox *sctpChunkCombo_ = nullptr;
	QComboBox *sccpMessageCombo_ = nullptr;
	QCheckBox *includeM3uaCheck_ = nullptr;
	QDialogButtonBox *buttonBox_ = nullptr;
};
