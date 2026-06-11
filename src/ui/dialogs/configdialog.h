#pragma once

#include <QDialog>

#include <functional>
#include <memory>

#include "../../core/appconfig.h"

namespace Ui
{
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
public:
	explicit ConfigDialog(const AppConfig &config, QWidget *parent = nullptr);
	~ConfigDialog() override;

	AppConfig config() const;
	void setOnSave(const std::function<void(const AppConfig &)> &onSave);
	void setOnApply(const std::function<void(const AppConfig &)> &onApply);

private:
	std::unique_ptr<Ui::ConfigDialog> ui_;
	std::function<void(const AppConfig &)> onSave_ {};
	std::function<void(const AppConfig &)> onApply_ {};
};
