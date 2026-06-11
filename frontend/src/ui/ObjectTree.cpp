#include "ui/ObjectTree.h"
#include "ui/Icons.h"
#include "backend/BackendClient.h"
#include "backend/FuncId.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>

ObjectTree::ObjectTree(BackendClient *client, QWidget *parent)
    : QTreeWidget(parent), client_(client) {
    setHeaderHidden(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeWidget::itemExpanded, this, &ObjectTree::onItemExpanded);
    connect(this, &QTreeWidget::itemDoubleClicked, this, &ObjectTree::onItemDoubleClicked);
    connect(this, &QTreeWidget::customContextMenuRequested, this, &ObjectTree::showContextMenu);
}

void ObjectTree::showContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = itemAt(pos);
    if (!item || item->data(0, NodeTypeRole).toInt() != Table) return;
    const QString connId = item->data(0, ConnIdRole).toString();
    const QString db = item->data(0, DbRole).toString();
    const QString table = item->text(0);
    QMenu menu(this);
    menu.addAction(tr("打开数据"), this, [=]{ emit tableActivated(connId, db, table); });
    menu.addAction(tr("查看结构"), this, [=]{ emit structureRequested(connId, db, table); });
    menu.exec(viewport()->mapToGlobal(pos));
}

bool ObjectTree::addConnection(const ConnData &c) {
    if (!client_) return false;
    auto r = client_->call(c.toOpenRequest(), 10000);
    if (!r.ok) return false;

    auto *item = new QTreeWidgetItem(this);
    item->setText(0, c.connId.isEmpty() ? c.type : c.connId);
    item->setIcon(0, Icons::connection(c.type));
    item->setData(0, NodeTypeRole, Conn);
    item->setData(0, ConnIdRole, c.connId);
    item->setData(0, ConnDataRole, QVariant::fromValue(c));
    item->setData(0, OpenedRole, true);
    addTopLevelItem(item);
    loadDatabases(item, c.connId);
    item->setExpanded(true);
    return true;
}

void ObjectTree::addSavedConnection(const ConnData &c) {
    auto *item = new QTreeWidgetItem(this);
    item->setText(0, c.connId.isEmpty() ? c.type : c.connId);
    item->setIcon(0, Icons::connection(c.type));
    item->setData(0, NodeTypeRole, Conn);
    item->setData(0, ConnIdRole, c.connId);
    item->setData(0, ConnDataRole, QVariant::fromValue(c));
    item->setData(0, OpenedRole, false);
    addTopLevelItem(item);
    // 占位子节点,使其可展开;展开时再真正连接
    new QTreeWidgetItem(item, QStringList(tr("(展开以连接)")));
}

QList<ConnData> ObjectTree::allConnections() const {
    QList<ConnData> out;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QVariant v = topLevelItem(i)->data(0, ConnDataRole);
        if (v.canConvert<ConnData>()) out << v.value<ConnData>();
    }
    return out;
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
        dbItem->setIcon(0, Icons::database());
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
        tItem->setIcon(0, Icons::table());
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
    int type = item->data(0, NodeTypeRole).toInt();
    if (type == Conn) {
        if (item->data(0, OpenedRole).toBool()) return;
        QVariant v = item->data(0, ConnDataRole);
        if (!v.canConvert<ConnData>()) return;
        ConnData c = v.value<ConnData>();
        auto r = client_->call(c.toOpenRequest(), 10000);
        // 清掉占位子节点
        for (auto *child : item->takeChildren()) delete child;
        if (!r.ok) {
            new QTreeWidgetItem(item, QStringList(tr("(连接失败: %1)").arg(r.errorMessage)));
            return;
        }
        item->setData(0, OpenedRole, true);
        loadDatabases(item, c.connId);
        return;
    }
    if (type != Db) return;
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
