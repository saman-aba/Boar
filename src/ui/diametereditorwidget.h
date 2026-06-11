#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class DiameterEditorWidget;
}

class DiameterEditorWidget : public QWidget
{
public:
	explicit DiameterEditorWidget(QWidget *parent = nullptr);
	~DiameterEditorWidget() override;

private:
	std::unique_ptr<Ui::DiameterEditorWidget> ui_;
};
