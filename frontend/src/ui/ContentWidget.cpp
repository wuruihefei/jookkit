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
#include <QToolButton>
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

    // 左:对象树(对象操作已在顶部工具栏与右键菜单,故不再重复加小工具栏)
    leftPanel_ = new QWidget;
    auto *lv = new QVBoxLayout(leftPanel_);
    lv->setContentsMargins(0, 0, 0, 0);
    lv->setSpacing(0);
    lv->addWidget(tree_, 1);

    // 右:标签页 +(可折叠、默认最小化的)信息窗格
    infoToggle_ = new QToolButton;
    infoToggle_->setAutoRaise(true);
    infoToggle_->setCheckable(true);
    infoToggle_->setToolButtonStyle(Qt::ToolButtonTextOnly);
    infoToggle_->setText(tr(" ▸ 信息"));
    connect(infoToggle_, &QToolButton::toggled, this, &ContentWidget::setInfoVisible);

    auto *infoHdr = new QToolBar;
    infoHdr->setObjectName("objectToolBar");
    infoHdr->addWidget(infoToggle_);

    auto *infoWrap = new QWidget;
    auto *iv = new QVBoxLayout(infoWrap);
    iv->setContentsMargins(0, 0, 0, 0);
    iv->setSpacing(0);
    iv->addWidget(infoHdr);
    iv->addWidget(info_);
    info_->setVisible(false);   // 默认最小化(只留标题条)

    auto *rightSplit = new QSplitter(Qt::Vertical);
    rightSplit->addWidget(tabs_);
    rightSplit->addWidget(infoWrap);
    rightSplit->setStretchFactor(0, 5);
    rightSplit->setStretchFactor(1, 0);

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

ConnData ContentWidget::currentConnData() const {
    QString id = tree_->currentConnId();
    for (const ConnData &c : tree_->allConnections())
        if (c.connId == id) return c;
    return ConnData();
}

QueryForm *ContentWidget::currentQueryForm() const {
    return qobject_cast<QueryForm *>(tabs_->currentWidget());
}

void ContentWidget::updateInfo() {
    if (!info_->isVisible()) return;   // 最小化时不拉取,省开销
    QString table = tree_->currentTable();
    if (table.isEmpty()) info_->clearInfo();
    else info_->showTable(tree_->currentConnId(), tree_->currentDb(), table);
}

bool ContentWidget::isSidebarVisible() const { return leftPanel_->isVisible(); }
bool ContentWidget::isInfoVisible() const { return info_->isVisible(); }
void ContentWidget::setSidebarVisible(bool on) { leftPanel_->setVisible(on); }
void ContentWidget::setInfoVisible(bool on) {
    info_->setVisible(on);
    if (infoToggle_) {
        infoToggle_->setChecked(on);
        infoToggle_->setText(on ? tr(" ▾ 信息") : tr(" ▸ 信息"));
    }
    if (on) updateInfo();   // 展开时按当前选中刷新
}
void ContentWidget::toggleSidebar() { setSidebarVisible(!isSidebarVisible()); }
void ContentWidget::toggleInfo() { setInfoVisible(!isInfoVisible()); }

void ContentWidget::addTab(QWidget *w, const QString &title) {
    int idx = tabs_->addTab(w, title);
    tabs_->setCurrentIndex(idx);
}

void ContentWidget::newQuery() {
    QList<ConnData> conns = tree_->allConnections();
    if (conns.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先新建一个连接"));
        return;
    }
    QString connId = tree_->currentConnId();
    if (connId.isEmpty()) connId = conns.first().connId;
    addTab(new QueryForm(client_, conns, connId, tree_->currentDb()), tr("查询"));
}

void ContentWidget::openTableData(const QString &connId, const QString &db, const QString &table) {
    addTab(new TableDataForm(client_, connId, db, table), tr("数据: %1").arg(table));
}

void ContentWidget::openTableStructure(const QString &connId, const QString &db, const QString &table) {
    QString dbType;
    for (const ConnData &c : tree_->allConnections())
        if (c.connId == connId) { dbType = c.type; break; }
    addTab(new TableStructureForm(client_, connId, db, table, dbType),
           tr("结构: %1").arg(table));
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
