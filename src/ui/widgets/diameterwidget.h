#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class DiameterWidget;
}

class DiameterWidget : public QWidget
{
public:
	explicit DiameterWidget(QWidget *parent = nullptr);
	~DiameterWidget() override;

private:
	std::unique_ptr<Ui::DiameterWidget> ui_;
};
