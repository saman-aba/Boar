#pragma once

#include <QDialog>

#include <memory>

namespace Ui
{
class AddAvpDialog;
}

class AddAvpDialog : public QDialog
{
public:
	explicit AddAvpDialog(QWidget *parent = nullptr);
	~AddAvpDialog() override;

	QString avpDisplayText() const;

private:
	std::unique_ptr<Ui::AddAvpDialog> ui_;
};
