#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class TcapEditorWidget;
}

class TcapEditorWidget : public QWidget
{
public:
	explicit TcapEditorWidget(QWidget *parent = nullptr);
	~TcapEditorWidget() override;

private:
	std::unique_ptr<Ui::TcapEditorWidget> ui_;
};
