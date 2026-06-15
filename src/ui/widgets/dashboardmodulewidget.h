#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class DashboardModuleWidget;
}

class DashboardModuleWidget : public QWidget
{
public:
	explicit DashboardModuleWidget(QWidget *parent = nullptr);
	~DashboardModuleWidget() override;

private:
	std::unique_ptr<Ui::DashboardModuleWidget> ui_;
};
