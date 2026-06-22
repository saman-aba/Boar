#pragma once

#include <QDialog>
#include <QHash>
#include <QStringList>

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
	void setParameterDetails(const QStringList &options, const QHash<QString, QString> &typeNames);
	QString selectedParameter() const;
	QString selectedTypeName() const;
	QString parameterValue() const;

private:
	void refreshTypeName();

	std::unique_ptr<Ui::AddTcapParameterDialog> ui_;
	QHash<QString, QString> typeNames_;
};
