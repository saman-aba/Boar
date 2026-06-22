#pragma once

#include <QDialog>

#include <memory>

class QString;

namespace Ui
{
class AddTcapParameterDialog;
}

class AddTcapParameterDialog : public QDialog
{
public:
	explicit AddTcapParameterDialog(QWidget *parent = nullptr);
	~AddTcapParameterDialog() override;

	void setParameterOptions(const QStringList &options);
	QString selectedParameter() const;
	QString parameterValue() const;

private:
	std::unique_ptr<Ui::AddTcapParameterDialog> ui_;
};
