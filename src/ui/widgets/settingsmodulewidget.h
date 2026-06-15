#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class SettingsModuleWidget;
}

class SettingsModuleWidget : public QWidget
{
public:
	explicit SettingsModuleWidget(QWidget *parent = nullptr);
	~SettingsModuleWidget() override;

private:
	std::unique_ptr<Ui::SettingsModuleWidget> ui_;
};
