#pragma once

#include <QDialog>

#include <memory>

namespace Ui
{
class NewPacketDialog;
}

class NewPacketDialog : public QDialog
{
public:
	explicit NewPacketDialog(QWidget *parent = nullptr);
	~NewPacketDialog() override;

private:
	std::unique_ptr<Ui::NewPacketDialog> ui_;
};
