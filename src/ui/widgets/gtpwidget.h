#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class GtpWidget;
}

class GtpWidget : public QWidget
{
public:
	explicit GtpWidget(QWidget *parent = nullptr);
	~GtpWidget() override;

private:
	std::unique_ptr<Ui::GtpWidget> ui_;
};
