#pragma once

#include <QDialog>

#include <QByteArray>

#include <memory>

class QTreeWidgetItem;

namespace Ui
{
class EditAvpDialog;
}

class EditAvpDialog : public QDialog
{
public:
	explicit EditAvpDialog(QTreeWidgetItem *item, QWidget *parent = nullptr);
	~EditAvpDialog() override;

	QString avpName() const;
	int avpCode() const;
	QString avpType() const;
	QString avpDerivedType() const;
	QString avpValue() const;
	QString avpFlags() const;

private:
	void updateLength();
	void updateDerivedTypeOptions();
	void editChildAvp(QTreeWidgetItem *item);
	void syncChildrenToItem();
	std::unique_ptr<Ui::EditAvpDialog> ui_;
	QTreeWidgetItem *item_ = nullptr;
};
