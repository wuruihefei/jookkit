#include "ui/ObjectTree.h"
#include "backend/BackendClient.h"
#include "backend/FuncId.h"

#include <QJsonObject>
#include <QJsonArray>

ObjectTree::ObjectTree(BackendClient *client, QWidget *parent)
    : QTreeWidget(parent), client_(client) {
    setHeaderHidden(true);
    connect(this, &QTreeWidget::itemExpanded, this, &ObjectTree::onItemExpanded);
    connect(this, &QTreeWidget::itemDoubleClicked, this, &ObjectTree::onItemDoubleClicked);
}

bool ObjectTree::addConnection(const ConnData &c) {
    if (!client_) return false;
    auto r = client_->call(c.toOpenRequest(), 10000);
    if (!r.ok) return false;

    auto *item = new QTreeWidgetItem(this);
    item->setText(0, c.connId.isEmpty() ? c.type : c.connId);
    item->setData(0, NodeTypeRole, Conn);
    item->setData(0, ConnIdRole, c.connId);
    addTopLevelItem(item);
    loadDatabases(item, c.connId);
    item->setExpanded(true);
    return true;
}

void ObjectTree::loadDatabases(QTreeWidgetItem *connItem, const QString &connId) {
    QJsonObject req;
    req.insert("funcId", FuncId::LIST_DATABASES);
    req.insert("connId", connId);
    auto r = client_->call(req);
    if (!r.ok) return;
    for (const auto &d : r.data.value("databases").toArray()) {
        auto *dbItem = new QTreeWidgetItem(connItem);
        dbItem->setText(0, d.toString());
        dbItem->setData(0, NodeTypeRole, Db);
        dbItem->setData(0, ConnIdRole, connId);
        dbItem->setData(0, DbRole, d.toString());
        dbItem->setData(0, LoadedRole, false);
        // 占位子节点,使其可展开
        new QTreeWidgetItem(dbItem, QStringList("(展开加载表)"));
    }
}

void ObjectTree::loadTables(QTreeWidgetItem *dbItem, const QString &connId, const QString &db) {
    QJsonObject req;
    req.insert("funcId", FuncId::LIST_TABLES);
    req.insert("connId", connId);
    req.insert("db", db);
    auto r = client_->call(req);
    // 清掉占位/旧节点
    for (auto *child : dbItem->takeChildren()) delete child;
    if (!r.ok) return;
    for (const auto &t : r.data.value("tables").toArray()) {
        auto *tItem = new QTreeWidgetItem(dbItem);
        tItem->setText(0, t.toString());
        tItem->setData(0, NodeTypeRole, Table);
        tItem->setData(0, ConnIdRole, connId);
        tItem->setData(0, DbRole, db);
    }
}

QString ObjectTree::currentConnId() const {
    QTreeWidgetItem *it = currentItem();
    return it ? it->data(0, ConnIdRole).toString() : QString();
}

QString ObjectTree::currentDb() const {
    QTreeWidgetItem *it = currentItem();
    return it ? it->data(0, DbRole).toString() : QString();
}

QString ObjectTree::currentTable() const {
    QTreeWidgetItem *it = currentItem();
    if (it && it->data(0, NodeTypeRole).toInt() == Table) return it->text(0);
    return QString();
}

void ObjectTree::onItemExpanded(QTreeWidgetItem *item) {
    if (item->data(0, NodeTypeRole).toInt() != Db) return;
    if (item->data(0, LoadedRole).toBool()) return;
    item->setData(0, LoadedRole, true);
    loadTables(item, item->data(0, ConnIdRole).toString(),
               item->data(0, DbRole).toString());
}

void ObjectTree::onItemDoubleClicked(QTreeWidgetItem *item, int) {
    if (item->data(0, NodeTypeRole).toInt() != Table) return;
    emit tableActivated(item->data(0, ConnIdRole).toString(),
                        item->data(0, DbRole).toString(),
                        item->text(0));
}
