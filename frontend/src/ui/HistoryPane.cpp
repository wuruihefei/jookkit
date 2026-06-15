#include "ui/HistoryPane.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>

HistoryPane::HistoryPane(QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    search_ = new QLineEdit(this);
    search_->setPlaceholderText("搜索 SQL …");
    list_ = new QListWidget(this);
    list_->setContextMenuPolicy(Qt::CustomContextMenu);
    lay->addWidget(search_);
    lay->addWidget(list_);

    connect(search_, &QLineEdit::textChanged, this, &HistoryPane::applyFilter);
    connect(list_, &QListWidget::itemDoubleClicked, this, &HistoryPane::onItemActivated);
    connect(list_, &QListWidget::customContextMenuRequested, this, &HistoryPane::showMenu);

    refresh();
}

void HistoryPane::refresh() {
    entries_ = HistoryStore::load(1000);
    rebuild();
}

void HistoryPane::applyFilter(const QString &text) {
    filter_ = text;
    rebuild();
}

void HistoryPane::rebuild() {
    list_->clear();
    for (const HistoryEntry &e : entries_) {
        if (!filter_.isEmpty() && !e.sql.contains(filter_, Qt::CaseInsensitive))
            continue;
        const QString when = QDateTime::fromMSecsSinceEpoch(e.ts).toString("MM-dd HH:mm:ss");
        const QString flag = e.ok ? "✓" : "✗";
        QString oneLine = e.sql;
        oneLine.replace('\n', ' ');
        if (oneLine.size() > 80) oneLine = oneLine.left(80) + "…";
        auto *it = new QListWidgetItem(
            QString("%1 [%2] %3").arg(flag, when, oneLine), list_);
        it->setData(Qt::UserRole, e.sql);
        it->setToolTip(e.ok ? e.sql : (e.sql + "\n-- 错误: " + e.error));
    }
}

void HistoryPane::onItemActivated() {
    auto *it = list_->currentItem();
    if (it) emit sqlChosen(it->data(Qt::UserRole).toString());
}

void HistoryPane::showMenu(const QPoint &pos) {
    auto *it = list_->itemAt(pos);
    QMenu menu(this);
    QAction *send = menu.addAction("送回编辑器");
    QAction *copy = menu.addAction("复制 SQL");
    menu.addSeparator();
    QAction *clr = menu.addAction("清空历史");
    send->setEnabled(it != nullptr);
    copy->setEnabled(it != nullptr);
    QAction *chosen = menu.exec(list_->mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == send && it) emit sqlChosen(it->data(Qt::UserRole).toString());
    else if (chosen == copy && it)
        QApplication::clipboard()->setText(it->data(Qt::UserRole).toString());
    else if (chosen == clr) { HistoryStore::clear(); refresh(); }
}
