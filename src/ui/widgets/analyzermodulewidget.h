#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class AnalyzerModuleWidget;
}

class AnalyzerModuleWidget : public QWidget
{
public:
	explicit AnalyzerModuleWidget(QWidget *parent = nullptr);
	~AnalyzerModuleWidget() override;

private:
	std::unique_ptr<Ui::AnalyzerModuleWidget> ui_;
};
