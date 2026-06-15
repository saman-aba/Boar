#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class DatabaseModuleWidget;
}

class DatabaseModuleWidget : public QWidget
{
public:
	explicit DatabaseModuleWidget(QWidget *parent = nullptr);
	~DatabaseModuleWidget() override;

private:
	std::unique_ptr<Ui::DatabaseModuleWidget> ui_;
};
