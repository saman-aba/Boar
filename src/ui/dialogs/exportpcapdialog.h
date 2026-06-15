#pragma once

#include <QDialog>

#include <memory>

namespace Ui
{
class ExportPcapDialog;
}

class ExportPcapDialog : public QDialog
{
public:
	explicit ExportPcapDialog(QWidget *parent = nullptr);
	~ExportPcapDialog() override;

	QString exportDirectory() const;

private:
	std::unique_ptr<Ui::ExportPcapDialog> ui_;
};
