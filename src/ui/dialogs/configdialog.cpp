#include "configdialog.h"

#include <QComboBox>
#include <QPushButton>

#include "ui_ConfigDialog.h"

ConfigDialog::ConfigDialog(const AppConfig &config, QWidget *parent)
	: QDialog(parent)
	, ui_(std::make_unique<Ui::ConfigDialog>())
{
	ui_->setupUi(this);
	ui_->windowWidthSpinBox->setRange(320, 3840);
	ui_->windowHeightSpinBox->setRange(240, 2160);
	ui_->payloadBoxHeightSpinBox->setRange(80, 1080);
	ui_->sendButtonWidthSpinBox->setRange(80, 400);
	ui_->portSpinBox->setRange(1, 65535);
	ui_->themeComboBox->addItems({QStringLiteral("System"), QStringLiteral("Light"), QStringLiteral("Dark")});
	ui_->lastPayloadEdit->setText(config.lastPayload);
	ui_->configFilePathEdit->setText(config.configFilePath);
	ui_->hostEdit->setText(config.host);
	ui_->portSpinBox->setValue(config.port);
	const int themeIndex = ui_->themeComboBox->findText(config.theme);
	if (themeIndex >= 0) {
		ui_->themeComboBox->setCurrentIndex(themeIndex);
	}
	connect(ui_->saveButton, &QPushButton::clicked, this, [this]() {
		if (onSave_) {
			onSave_(this->config());
		}
	});
	connect(ui_->applyButton, &QPushButton::clicked, this, [this]() {
		if (onApply_) {
			onApply_(this->config());
		}
	});
	connect(ui_->closeButton, &QPushButton::clicked, this, &QDialog::close);
}

ConfigDialog::~ConfigDialog() = default;

AppConfig ConfigDialog::config() const
{
	AppConfig config;
	config.lastPayload = ui_->lastPayloadEdit->text();
	config.host = ui_->hostEdit->text();
	config.port = ui_->portSpinBox->value();
	config.theme = ui_->themeComboBox->currentText();
	config.configFilePath = ui_->configFilePathEdit->text().isEmpty() ? AppConfig::defaultConfigFilePath() : ui_->configFilePathEdit->text();
	return config;
}

void ConfigDialog::setOnSave(const std::function<void(const AppConfig &)> &onSave)
{
	onSave_ = onSave;
}

void ConfigDialog::setOnApply(const std::function<void(const AppConfig &)> &onApply)
{
	onApply_ = onApply;
}
