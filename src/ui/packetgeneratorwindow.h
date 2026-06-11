#pragma once

#include <QWidget>

#include <memory>

namespace Ui
{
class PacketGeneratorWindow;
}

class PacketGeneratorWindow : public QWidget
{
public:
	explicit PacketGeneratorWindow(QWidget *parent = nullptr);
	~PacketGeneratorWindow() override;

private:
	std::unique_ptr<Ui::PacketGeneratorWindow> ui_;
};
