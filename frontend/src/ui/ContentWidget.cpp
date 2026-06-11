#include "ui/ContentWidget.h"
#include "ui/ObjectTree.h"
#include "ui/QueryForm.h"
#include "ui/TableDataForm.h"
#include "ui/TableStructureForm.h"

#include <QTabWidget>
#include <QSplitter>
#include <QHBoxLayout>
#include <QMessageBox>

ContentWidget::ContentWidget(BackendClient *client, QWidget *parent)
    : QWidget(parent), client_(client) {
    tree_ = new ObjectTree(client_);
    tabs_ = new QTabWidget;
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    connect(tabs_, &QTabWidget::tabCloseRequested, this, [this](int i){
        QWidget *w = tabs_->widget(i);
        tabs_->removeTab(i);
        w->deleteLater();
    });

    connect(tree_, &ObjectTree::tableActivated, this, &ContentWidget::openTableData);
    connect(tree_, &ObjectTree::structureRequested, this, &ContentWidget::openTableStructure);

    auto *split = new QSplitter;
    split->addWidget(tree_);
    split->addWidget(tabs_);
    split->setStretchFactor(1, 1);
    split->setSizes({250, 750});

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(split);
}

bool ContentWidget::addConnection(const ConnData &c) {
    return tree_->addConnection(c);
}

QueryForm *ContentWidget::currentQueryForm() const {
    return qobject_cast<QueryForm *>(tabs_->currentWidget());
}

void ContentWidget::addTab(QWidget *w, const QString &title) {
    int idx = tabs_->addTab(w, title);
    tabs_->setCurrentIndex(idx);
}

void ContentWidget::newQuery() {
    QString connId = tree_->currentConnId();
    if (connId.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先在左侧选择一个连接"));
        return;
    }
    addTab(new QueryForm(client_, connId, tree_->currentDb()),
           tr("查询 [%1]").arg(connId));
}

void ContentWidget::openTableData(const QString &connId, const QString &db, const QString &table) {
    addTab(new TableDataForm(client_, connId, db, table), tr("数据: %1").arg(table));
}

void ContentWidget::openTableStructure(const QString &connId, const QString &db, const QString &table) {
    addTab(new TableStructureForm(client_, connId, db, table), tr("结构: %1").arg(table));
}

void ContentWidget::viewCurrentData() {
    QString table = tree_->currentTable();
    if (table.isEmpty()) { QMessageBox::information(this, tr("提示"), tr("请先在左侧选择一个表")); return; }
    openTableData(tree_->currentConnId(), tree_->currentDb(), table);
}

void ContentWidget::viewCurrentStructure() {
    QString table = tree_->currentTable();
    if (table.isEmpty()) { QMessageBox::information(this, tr("提示"), tr("请先在左侧选择一个表")); return; }
    openTableStructure(tree_->currentConnId(), tree_->currentDb(), table);
}

void ContentWidget::closeCurrentTab() {
    int i = tabs_->currentIndex();
    if (i < 0) return;
    QWidget *w = tabs_->widget(i);
    tabs_->removeTab(i);
    w->deleteLater();
}

void ContentWidget::closeOtherTabs() {
    int keep = tabs_->currentIndex();
    if (keep < 0) return;
    QWidget *keepW = tabs_->widget(keep);
    for (int i = tabs_->count() - 1; i >= 0; --i) {
        if (tabs_->widget(i) != keepW) {
            QWidget *w = tabs_->widget(i);
            tabs_->removeTab(i);
            w->deleteLater();
        }
    }
}

void ContentWidget::closeAllTabs() {
    while (tabs_->count() > 0) {
        QWidget *w = tabs_->widget(0);
        tabs_->removeTab(0);
        w->deleteLater();
    }
}

void ContentWidget::previousTab() {
    int n = tabs_->count();
    if (n > 0) tabs_->setCurrentIndex((tabs_->currentIndex() - 1 + n) % n);
}

void ContentWidget::nextTab() {
    int n = tabs_->count();
    if (n > 0) tabs_->setCurrentIndex((tabs_->currentIndex() + 1) % n);
}

void ContentWidget::toggleSidebar() {
    tree_->setVisible(!tree_->isVisible());
}
