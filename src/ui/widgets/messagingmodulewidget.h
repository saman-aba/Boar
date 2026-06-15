#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class MessagingModuleWidget;
}

class MessagingModuleWidget : public QWidget
{
public:
	explicit MessagingModuleWidget(QWidget *parent = nullptr);
	~MessagingModuleWidget() override;

private:
	std::unique_ptr<Ui::MessagingModuleWidget> ui_;
};
