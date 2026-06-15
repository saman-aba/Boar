#include "editavpdialog.h"

#include <QComboBox>
#include <QDateTime>
#include <QHeaderView>
#include <QLineEdit>
#include <QTimeZone>
#include <QPushButton>
#include <QSpinBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "ui_EditAvpDialog.h"

EditAvpDialog::EditAvpDialog(QTreeWidgetItem *item, QWidget *parent)
	: QDialog(parent)
	, ui_(std::make_unique<Ui::EditAvpDialog>())
	, item_(item)
{
	ui_->setupUi(this);
	if (ui_->treeChildAvps) {
		ui_->treeChildAvps->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		if (auto *header = ui_->treeChildAvps->header()) {
			header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
			header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
			header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
			header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
			header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
			header->setSectionResizeMode(5, QHeaderView::Interactive);
		}
		ui_->treeChildAvps->setColumnWidth(5, 160);
	}
	ui_->comboAvpType->addItems({
		QStringLiteral("Unknown"),
		QStringLiteral("OctetString"),
		QStringLiteral("Integer32"),
		QStringLiteral("Integer64"),
		QStringLiteral("Unsigned32"),
		QStringLiteral("Unsigned64"),
		QStringLiteral("Float32"),
		QStringLiteral("Float64"),
		QStringLiteral("Grouped")
	});
	updateDerivedTypeOptions();
	if (item_) {
		ui_->editAvpName->setText(item_->text(0));
		ui_->spinAvpCode->setValue(item_->text(1).toInt());
		const int typeIndex = ui_->comboAvpType->findText(item_->data(0, Qt::UserRole).toString());
		ui_->comboAvpType->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
		updateDerivedTypeOptions();
		const QString derivedType = item_->data(0, Qt::UserRole + 1).toString();
		const int derivedIndex = ui_->comboDerivedType->findText(derivedType);
		ui_->comboDerivedType->setCurrentIndex(derivedIndex >= 0 ? derivedIndex : 0);
		const QString storedValue = item_->data(0, Qt::UserRole + 2).toString();
		const bool plainOctetString = item_->data(0, Qt::UserRole).toString() == QStringLiteral("OctetString")
			&& (derivedType.isEmpty() || derivedType == QStringLiteral("OctetString"));
		ui_->editAvpValue->setText(plainOctetString ? item_->text(5) : (storedValue.isEmpty() ? item_->text(5) : storedValue));
		ui_->checkVendor->setChecked(item_->text(2) == QStringLiteral("✓"));
		ui_->checkMandatory->setChecked(item_->text(3) == QStringLiteral("✓"));
		ui_->checkProtected->setChecked(item_->text(4) == QStringLiteral("✓"));
		for (int index = 0; index < item_->childCount(); ++index) {
			auto *child = item_->child(index)->clone();
			ui_->treeChildAvps->addTopLevelItem(child);
		}
	}
	connect(ui_->comboAvpType, &QComboBox::currentIndexChanged, this, [this]() {
		const bool grouped = ui_->comboAvpType->currentText() == QStringLiteral("Grouped");
		updateDerivedTypeOptions();
		ui_->groupChildAvps->setVisible(grouped);
		updateLength();
	});
	connect(ui_->comboDerivedType, &QComboBox::currentIndexChanged, this, [this]() {
		if (ui_->comboAvpType->currentText() != QStringLiteral("OctetString")) {
			return;
		}
		QString value = ui_->editAvpValue->text().trimmed();
		QString compactValue = value;
		compactValue.remove(QChar::fromLatin1(' '));
		const QString derivedType = ui_->comboDerivedType->currentText();
		const bool stringLike = derivedType == QStringLiteral("UTF8String")
			|| derivedType == QStringLiteral("DiameterIdentity")
			|| derivedType == QStringLiteral("DiameterURI")
			|| derivedType == QStringLiteral("IPFilterRule")
			|| derivedType == QStringLiteral("QoSFilterRule");
		const QByteArray decoded = QByteArray::fromHex(compactValue.toUtf8());
		const bool looksHex = !compactValue.isEmpty() && decoded.toHex() == compactValue.toLower().toUtf8();
		if (stringLike && looksHex) {
			ui_->editAvpValue->setText(QString::fromUtf8(decoded));
			return;
		}
		if (!stringLike && derivedType != QStringLiteral("None") && !looksHex) {
			ui_->editAvpValue->setText(QString::fromLatin1(value.toUtf8().toHex(' ')));
		}
	});
	connect(ui_->btnAddChildAvp, &QPushButton::clicked, this, [this]() {
		auto *child = new QTreeWidgetItem();
		child->setText(0, QStringLiteral("Child AVP"));
		child->setText(1, QStringLiteral("0"));
		child->setText(2, QStringLiteral(""));
		child->setText(3, QStringLiteral(""));
		child->setText(4, QStringLiteral(""));
		child->setText(5, QStringLiteral(""));
		child->setData(0, Qt::UserRole, QStringLiteral("Unknown"));
		child->setData(0, Qt::UserRole + 1, QStringLiteral("None"));
		child->setData(0, Qt::UserRole + 2, QStringLiteral(""));
		ui_->treeChildAvps->addTopLevelItem(child);
		editChildAvp(child);
		updateLength();
	});
	connect(ui_->btnRemoveChildAvp, &QPushButton::clicked, this, [this]() {
		delete ui_->treeChildAvps->takeTopLevelItem(ui_->treeChildAvps->currentIndex().row());
		updateLength();
	});
	connect(ui_->treeChildAvps, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item) {
		editChildAvp(item);
	});
	connect(ui_->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
	connect(ui_->btnSave, &QPushButton::clicked, this, [this]() {
		syncChildrenToItem();
		accept();
	});
	ui_->groupChildAvps->setVisible(ui_->comboAvpType->currentText() == QStringLiteral("Grouped"));
	updateLength();
}

EditAvpDialog::~EditAvpDialog() = default;

QString EditAvpDialog::avpName() const
{
	return ui_->editAvpName->text();
}

int EditAvpDialog::avpCode() const
{
	return ui_->spinAvpCode->value();
}

QString EditAvpDialog::avpType() const
{
	return ui_->comboAvpType->currentText();
}

QString EditAvpDialog::avpDerivedType() const
{
	return ui_->comboDerivedType->currentText();
}

QString EditAvpDialog::avpValue() const
{
	return ui_->editAvpValue->text();
}

QString EditAvpDialog::avpFlags() const
{
	QString flags;
	if (ui_->checkVendor->isChecked()) {
		flags += QStringLiteral("V");
	}
	if (ui_->checkMandatory->isChecked()) {
		flags += QStringLiteral("M");
	}
	if (ui_->checkProtected->isChecked()) {
		flags += QStringLiteral("P");
	}
	return flags;
}

void EditAvpDialog::editChildAvp(QTreeWidgetItem *item)
{
	if (!item) {
		return;
	}
	EditAvpDialog dialog(item, this);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}
	item->setText(0, dialog.avpName());
	item->setText(1, QString::number(dialog.avpCode()));
	item->setText(2, dialog.avpFlags().contains(QStringLiteral("V")) ? QStringLiteral("✓") : QStringLiteral(""));
	item->setText(3, dialog.avpFlags().contains(QStringLiteral("M")) ? QStringLiteral("✓") : QStringLiteral(""));
	item->setText(4, dialog.avpFlags().contains(QStringLiteral("P")) ? QStringLiteral("✓") : QStringLiteral(""));
	item->setText(5, dialog.avpValue());
	item->setData(0, Qt::UserRole, dialog.avpType());
	item->setData(0, Qt::UserRole + 1, dialog.avpDerivedType());
	item->setData(0, Qt::UserRole + 2, dialog.avpValue());
	updateLength();
}

void EditAvpDialog::updateDerivedTypeOptions()
{
	const QString currentDerivedType = ui_->comboDerivedType->currentText();
	ui_->comboDerivedType->clear();
	ui_->comboDerivedType->addItem(QStringLiteral("None"));
	const QString type = ui_->comboAvpType->currentText();
	if (type == QStringLiteral("OctetString")) {
		ui_->comboDerivedType->addItems({
			QStringLiteral("OctetString"),
			QStringLiteral("Address"),
			QStringLiteral("Time"),
			QStringLiteral("UTF8String"),
			QStringLiteral("DiameterIdentity"),
			QStringLiteral("DiameterURI"),
			QStringLiteral("IPFilterRule"),
			QStringLiteral("QoSFilterRule")
		});
	} else if (type == QStringLiteral("Integer32")) {
		ui_->comboDerivedType->addItems({ QStringLiteral("Integer32") });
	} else if (type == QStringLiteral("Integer64")) {
		ui_->comboDerivedType->addItems({ QStringLiteral("Integer64") });
	} else if (type == QStringLiteral("Unsigned32")) {
		ui_->comboDerivedType->addItems({ QStringLiteral("Unsigned32") });
	} else if (type == QStringLiteral("Unsigned64")) {
		ui_->comboDerivedType->addItems({ QStringLiteral("Unsigned64") });
	} else if (type == QStringLiteral("Float32")) {
		ui_->comboDerivedType->addItems({ QStringLiteral("Float32") });
	} else if (type == QStringLiteral("Float64")) {
		ui_->comboDerivedType->addItems({ QStringLiteral("Float64") });
	} else if (type == QStringLiteral("Grouped")) {
		ui_->comboDerivedType->addItems({ QStringLiteral("Grouped") });
	}
	const int derivedIndex = ui_->comboDerivedType->findText(currentDerivedType);
	ui_->comboDerivedType->setCurrentIndex(derivedIndex >= 0 ? derivedIndex : 0);
	const bool showDerived = ui_->comboDerivedType->count() > 1 || ui_->comboDerivedType->itemText(0) != QStringLiteral("None");
	ui_->labelDerivedType->setVisible(showDerived);
	ui_->comboDerivedType->setVisible(showDerived);
}

void EditAvpDialog::syncChildrenToItem()
{
	if (!item_) {
		return;
	}
	while (item_->childCount() > 0) {
		delete item_->takeChild(0);
	}
	if (ui_->comboAvpType->currentText() != QStringLiteral("Grouped")) {
		return;
	}
	for (int index = 0; index < ui_->treeChildAvps->topLevelItemCount(); ++index) {
		item_->addChild(ui_->treeChildAvps->topLevelItem(index)->clone());
	}
}

void EditAvpDialog::updateLength()
{
	int length = 8;
	if (ui_->comboAvpType->currentText() == QStringLiteral("Grouped")) {
		length += ui_->treeChildAvps->topLevelItemCount() * 8;
	} else {
		length += ui_->editAvpValue->text().size();
	}
}
