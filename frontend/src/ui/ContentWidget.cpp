#include "ui/ContentWidget.h"
#include "ui/ObjectTree.h"
#include "ui/QueryForm.h"
#include "ui/TableDataForm.h"
#include "ui/TableStructureForm.h"
#include "ui/InformationPane.h"
#include "ui/Icons.h"

#include <QTabWidget>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolBar>
#include <QTreeWidget>
#include <QMessageBox>

ContentWidget::ContentWidget(BackendClient *client, QWidget *parent)
    : QWidget(parent), client_(client) {
    tree_ = new ObjectTree(client_);
    tabs_ = new QTabWidget;
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    info_ = new InformationPane(client_);

    connect(tabs_, &QTabWidget::tabCloseRequested, this, [this](int i){
        QWidget *w = tabs_->widget(i);
        tabs_->removeTab(i);
        w->deleteLater();
    });
    connect(tree_, &ObjectTree::tableActivated, this, &ContentWidget::openTableData);
    connect(tree_, &ObjectTree::structureRequested, this, &ContentWidget::openTableStructure);
    connect(tree_, &QTreeWidget::currentItemChanged, this, &ContentWidget::updateInfo);

    // 左:对象工具栏 + 对象树
    auto *objBar = new QToolBar;
    objBar->setObjectName("objectToolBar");
    objBar->setIconSize(QSize(18, 18));
    objBar->addAction(Icons::data(), tr("打开数据"), this, &ContentWidget::viewCurrentData);
    objBar->addAction(Icons::structure(), tr("查看结构"), this, &ContentWidget::viewCurrentStructure);
    objBar->addAction(Icons::query(), tr("新建查询"), this, &ContentWidget::newQuery);

    leftPanel_ = new QWidget;
    auto *lv = new QVBoxLayout(leftPanel_);
    lv->setContentsMargins(0, 0, 0, 0);
    lv->setSpacing(0);
    lv->addWidget(objBar);
    lv->addWidget(tree_, 1);

    // 右:标签页 + 信息窗格
    auto *rightSplit = new QSplitter(Qt::Vertical);
    rightSplit->addWidget(tabs_);
    rightSplit->addWidget(info_);
    rightSplit->setStretchFactor(0, 3);
    rightSplit->setStretchFactor(1, 1);
    rightSplit->setSizes({500, 180});

    auto *mainSplit = new QSplitter;
    mainSplit->addWidget(leftPanel_);
    mainSplit->addWidget(rightSplit);
    mainSplit->setStretchFactor(1, 1);
    mainSplit->setSizes({250, 790});

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mainSplit);
}

bool ContentWidget::addConnection(const ConnData &c) { return tree_->addConnection(c); }
void ContentWidget::addSavedConnection(const ConnData &c) { tree_->addSavedConnection(c); }
QList<ConnData> ContentWidget::allConnections() const { return tree_->allConnections(); }

QueryForm *ContentWidget::currentQueryForm() const {
    return qobject_cast<QueryForm *>(tabs_->currentWidget());
}

void ContentWidget::updateInfo() {
    QString table = tree_->currentTable();
    if (table.isEmpty()) info_->clearInfo();
    else info_->showTable(tree_->currentConnId(), tree_->currentDb(), table);
}

bool ContentWidget::isSidebarVisible() const { return leftPanel_->isVisible(); }
bool ContentWidget::isInfoVisible() const { return info_->isVisible(); }
void ContentWidget::setSidebarVisible(bool on) { leftPanel_->setVisible(on); }
void ContentWidget::setInfoVisible(bool on) { info_->setVisible(on); }
void ContentWidget::toggleSidebar() { setSidebarVisible(!isSidebarVisible()); }
void ContentWidget::toggleInfo() { setInfoVisible(!isInfoVisible()); }

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
    addTab(new QueryForm(client_, connId, tree_->currentDb()), tr("查询 [%1]").arg(connId));
}

void ContentWidget::openTableData(const QString &connId, const QString &db, const QString &table) {
    addTab(new TableDataForm(client_, connId, db, table), tr("数据: %1").arg(table));
}

void ContentWidget::openTableStructure(const QString &connId, const QString &db, const QString &table) {
    addTab(new TableStructureForm(client_, connId, db, table), tr("结构: %1").arg(table));
}

void ContentWidget::viewCurrentData() {
    QString table = tree_->currentTable();
    if (table.isEmpty()) { QMessageBox::information(this, tr("提示"), tr("请先选择一个表")); return; }
    openTableData(tree_->currentConnId(), tree_->currentDb(), table);
}

void ContentWidget::viewCurrentStructure() {
    QString table = tree_->currentTable();
    if (table.isEmpty()) { QMessageBox::information(this, tr("提示"), tr("请先选择一个表")); return; }
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
