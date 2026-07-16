#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class ScenarioBuilderWidget;
}

class ScenarioBuilderWidget : public QWidget
{
public:
	explicit ScenarioBuilderWidget(QWidget *parent = nullptr);
	~ScenarioBuilderWidget() override;

private:
	std::unique_ptr<Ui::ScenarioBuilderWidget> ui_;
};
