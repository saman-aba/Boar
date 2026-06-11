#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class TcapWidget;
}

class TcapWidget : public QWidget
{
public:
	explicit TcapWidget(QWidget *parent = nullptr);
	~TcapWidget() override;

private:
	std::unique_ptr<Ui::TcapWidget> ui_;
};
