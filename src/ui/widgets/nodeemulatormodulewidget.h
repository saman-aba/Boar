#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class NodeEmulatorModuleWidget;
}

class NodeEmulatorModuleWidget : public QWidget
{
public:
	explicit NodeEmulatorModuleWidget(QWidget *parent = nullptr);
	~NodeEmulatorModuleWidget() override;

private:
	std::unique_ptr<Ui::NodeEmulatorModuleWidget> ui_;
};
