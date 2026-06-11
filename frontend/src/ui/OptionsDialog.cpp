#include "ui/OptionsDialog.h"

#include <QComboBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QSettings>
#include <QLabel>

OptionsDialog::OptionsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("选项"));
    QSettings s;

    pageSize_ = new QComboBox;
    pageSize_->addItems({"20", "100", "200", "500", "1000"});
    int ps = s.value("data/pageSize", 20).toInt();
    int idx = pageSize_->findText(QString::number(ps));
    pageSize_->setCurrentIndex(idx >= 0 ? idx : 0);

    fontSize_ = new QSpinBox;
    fontSize_->setRange(8, 24);
    fontSize_->setValue(s.value("editor/fontSize", 10).toInt());

    auto *form = new QFormLayout;
    form->addRow(tr("默认每页条数:"), pageSize_);
    form->addRow(tr("编辑器字体大小:"), fontSize_);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &OptionsDialog::apply);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    auto *hint = new QLabel(tr("(设置对新打开的标签生效)"));
    hint->setStyleSheet("color:#94a3b8;");
    layout->addWidget(hint);
    layout->addWidget(buttons);
}

void OptionsDialog::apply() {
    QSettings s;
    s.setValue("data/pageSize", pageSize_->currentText().toInt());
    s.setValue("editor/fontSize", fontSize_->value());
    accept();
}
