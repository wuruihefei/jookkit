#include "ui/FindReplaceDialog.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextDocument>
#include <QTextCursor>

FindReplaceDialog::FindReplaceDialog(QPlainTextEdit *editor, QWidget *parent)
    : QDialog(parent), editor_(editor) {
    setWindowTitle(tr("查找替换"));

    find_ = new QLineEdit;
    replace_ = new QLineEdit;
    caseChk_ = new QCheckBox(tr("区分大小写"));

    auto *form = new QFormLayout;
    form->addRow(tr("查找:"), find_);
    form->addRow(tr("替换为:"), replace_);

    auto *findBtn = new QPushButton(tr("查找下一个"));
    auto *replBtn = new QPushButton(tr("替换"));
    auto *replAllBtn = new QPushButton(tr("全部替换"));
    connect(findBtn, &QPushButton::clicked, this, &FindReplaceDialog::findNext);
    connect(replBtn, &QPushButton::clicked, this, &FindReplaceDialog::replaceOne);
    connect(replAllBtn, &QPushButton::clicked, this, &FindReplaceDialog::replaceAll);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(findBtn);
    btnRow->addWidget(replBtn);
    btnRow->addWidget(replAllBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(caseChk_);
    layout->addLayout(btnRow);
}

void FindReplaceDialog::findNext() {
    if (!editor_ || find_->text().isEmpty()) return;
    QTextDocument::FindFlags flags;
    if (caseChk_->isChecked()) flags |= QTextDocument::FindCaseSensitively;
    if (!editor_->find(find_->text(), flags)) {
        // 回到开头再找一次
        QTextCursor c = editor_->textCursor();
        c.movePosition(QTextCursor::Start);
        editor_->setTextCursor(c);
        editor_->find(find_->text(), flags);
    }
}

void FindReplaceDialog::replaceOne() {
    if (!editor_) return;
    QTextCursor c = editor_->textCursor();
    if (c.hasSelection() &&
        (caseChk_->isChecked() ? c.selectedText() == find_->text()
                               : c.selectedText().compare(find_->text(), Qt::CaseInsensitive) == 0)) {
        c.insertText(replace_->text());
    }
    findNext();
}

void FindReplaceDialog::replaceAll() {
    if (!editor_ || find_->text().isEmpty()) return;
    QString text = editor_->toPlainText();
    text.replace(find_->text(), replace_->text(),
                 caseChk_->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    editor_->setPlainText(text);
}
