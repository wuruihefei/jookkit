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
    connect(tabs_, &QTabWidget::tabCloseRequested, this, [this](int i){
        QWidget *w = tabs_->widget(i);
        tabs_->removeTab(i);
        w->deleteLater();
    });

    connect(tree_, &ObjectTree::tableActivated, this, &ContentWidget::openTableData);

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
    addTab(new TableDataForm(client_, connId, db, table),
           tr("数据: %1").arg(table));
}

void ContentWidget::viewCurrentData() {
    QString table = tree_->currentTable();
    if (table.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先在左侧选择一个表"));
        return;
    }
    openTableData(tree_->currentConnId(), tree_->currentDb(), table);
}

void ContentWidget::viewCurrentStructure() {
    QString table = tree_->currentTable();
    if (table.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先在左侧选择一个表"));
        return;
    }
    addTab(new TableStructureForm(client_, tree_->currentConnId(),
                                  tree_->currentDb(), table),
           tr("结构: %1").arg(table));
}
