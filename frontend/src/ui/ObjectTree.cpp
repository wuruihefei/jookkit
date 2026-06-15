#include "ui/ObjectTree.h"
#include "ui/Icons.h"
#include "ui/ConnDialog.h"
#include "backend/BackendClient.h"
#include "backend/FuncId.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>
#include <QMessageBox>

namespace {
// 连接节点悬停提示:类型 + 地址,便于区分同名/多个连接
QString connTip(const ConnData &c) {
    if (c.type == "sqlite")
        return QStringLiteral("SQLite  %1").arg(c.file);
    return QStringLiteral("%1  %2:%3").arg(c.type.toUpper(), c.host).arg(c.port);
}
}

ObjectTree::ObjectTree(BackendClient *client, QWidget *parent)
    : QTreeWidget(parent), client_(client) {
    setHeaderHidden(true);
    setIconSize(QSize(18, 18));  // 默认 16 偏小,状态点/类型色不易辨认
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeWidget::itemExpanded, this, &ObjectTree::onItemExpanded);
    connect(this, &QTreeWidget::itemDoubleClicked, this, &ObjectTree::onItemDoubleClicked);
    connect(this, &QTreeWidget::customContextMenuRequested, this, &ObjectTree::showContextMenu);
}

void ObjectTree::showContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = itemAt(pos);
    if (!item) return;
    const int type = item->data(0, NodeTypeRole).toInt();
    QMenu menu(this);
    if (type == Conn) {
        if (item->data(0, OpenedRole).toBool())
            menu.addAction(Icons::plugOff(), tr("关闭连接"), this, [=]{ closeConnection(item); });
        else
            menu.addAction(Icons::plugOn(), tr("打开连接"), this, [=]{ item->setExpanded(true); });
        menu.addSeparator();
        menu.addAction(Icons::edit(), tr("编辑连接..."), this, [=]{ editConnection(item); });
        menu.addAction(Icons::copy(), tr("复制连接"), this, [=]{ duplicateConnection(item); });
        menu.addSeparator();
        menu.addAction(Icons::trash(), tr("删除连接"), this, [=]{ removeConnection(item); });
    } else if (type == Table) {
        const QString connId = item->data(0, ConnIdRole).toString();
        const QString db = item->data(0, DbRole).toString();
        const QString table = item->text(0);
        menu.addAction(Icons::data(), tr("打开数据"), this, [=]{ emit tableActivated(connId, db, table); });
        menu.addAction(Icons::structure(), tr("查看结构"), this, [=]{ emit structureRequested(connId, db, table); });
        menu.addSeparator();
        menu.addAction(Icons::add(), tr("导入数据到此表..."), this, [=]{ emit dataImportRequested(connId, db, table); });
    } else {
        return;
    }
    menu.exec(viewport()->mapToGlobal(pos));
}

bool ObjectTree::connIdExists(const QString &id, const QTreeWidgetItem *except) const {
    for (int i = 0; i < topLevelItemCount(); ++i) {
        const QTreeWidgetItem *it = topLevelItem(i);
        if (it != except && it->data(0, ConnIdRole).toString() == id) return true;
    }
    return false;
}

QString ObjectTree::uniqueConnId(const QString &base) const {
    if (!connIdExists(base)) return base;
    for (int n = 2; ; ++n) {
        const QString cand = base + QString::number(n);
        if (!connIdExists(cand)) return cand;
    }
}

void ObjectTree::closeBackendConn(const QString &connId) {
    if (!client_) return;
    QJsonObject req;
    req.insert("funcId", FuncId::CLOSE_CONNECTION);
    req.insert("connId", connId);
    client_->call(req);  // 失败也无妨(可能本就未打开)
}

void ObjectTree::resetToClosed(QTreeWidgetItem *item, const ConnData &c) {
    item->setExpanded(false);
    for (auto *child : item->takeChildren()) delete child;
    new QTreeWidgetItem(item, QStringList(tr("(展开以连接)")));
    item->setText(0, c.connId.isEmpty() ? c.type : c.connId);
    item->setIcon(0, Icons::connection(c.type, false));
    item->setToolTip(0, connTip(c));
    item->setData(0, ConnIdRole, c.connId);
    item->setData(0, ConnDataRole, QVariant::fromValue(c));
    item->setData(0, OpenedRole, false);
}

void ObjectTree::closeConnection(QTreeWidgetItem *item) {
    QVariant v = item->data(0, ConnDataRole);
    if (!v.canConvert<ConnData>()) return;
    const ConnData c = v.value<ConnData>();
    closeBackendConn(c.connId);
    resetToClosed(item, c);
}

void ObjectTree::editConnection(QTreeWidgetItem *item) {
    QVariant v = item->data(0, ConnDataRole);
    if (!v.canConvert<ConnData>()) return;
    const ConnData old = v.value<ConnData>();

    // 编辑前必须断开:避免改参数时连接仍挂在旧会话上
    if (item->data(0, OpenedRole).toBool()) {
        if (QMessageBox::question(this, tr("编辑连接"),
                tr("编辑前需要先关闭连接 \"%1\"。\n关闭并继续编辑?").arg(old.connId))
            != QMessageBox::Yes) return;
        closeConnection(item);
    }

    ConnDialog dlg(client_, this);
    dlg.setConnData(old);
    if (dlg.exec() != QDialog::Accepted) return;
    ConnData c = dlg.connData();
    if (c.connId.isEmpty()) {
        QMessageBox::warning(this, tr("编辑连接"), tr("连接名不能为空"));
        return;
    }
    if (connIdExists(c.connId, item)) {
        QMessageBox::warning(this, tr("编辑连接"), tr("连接名 \"%1\" 已存在").arg(c.connId));
        return;
    }

    resetToClosed(item, c);  // 应用新配置,保持未连接,展开时按新配置重连
    emit connectionsChanged();
}

void ObjectTree::duplicateConnection(QTreeWidgetItem *item) {
    QVariant v = item->data(0, ConnDataRole);
    if (!v.canConvert<ConnData>()) return;
    ConnData c = v.value<ConnData>();
    c.connId = uniqueConnId(c.connId + tr("_副本"));
    addSavedConnection(c);
    emit connectionsChanged();
}

void ObjectTree::removeConnection(QTreeWidgetItem *item) {
    const QString id = item->data(0, ConnIdRole).toString();
    if (QMessageBox::question(this, tr("删除连接"),
            tr("确定删除连接 \"%1\" 吗?\n仅删除连接配置,不影响数据库数据。").arg(id))
        != QMessageBox::Yes) return;
    if (item->data(0, OpenedRole).toBool()) closeBackendConn(id);
    delete item;
    emit connectionsChanged();
}

bool ObjectTree::addConnection(const ConnData &c) {
    if (!client_) return false;
    auto r = client_->call(c.toOpenRequest(), 10000);
    if (!r.ok) return false;

    auto *item = new QTreeWidgetItem(this);
    item->setText(0, c.connId.isEmpty() ? c.type : c.connId);
    item->setIcon(0, Icons::connection(c.type, true));
    item->setToolTip(0, connTip(c));
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
    item->setIcon(0, Icons::connection(c.type, false));  // 未连接:灰显
    item->setToolTip(0, connTip(c));
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

bool ObjectTree::ensureOpen(const QString &connId) {
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = topLevelItem(i);
        if (item->data(0, ConnIdRole).toString() != connId) continue;
        if (item->data(0, OpenedRole).toBool()) return true;
        QVariant v = item->data(0, ConnDataRole);
        if (!v.canConvert<ConnData>() || !client_) return false;
        const ConnData c = v.value<ConnData>();
        auto r = client_->call(c.toOpenRequest(), 10000);
        if (!r.ok) return false;
        for (auto *child : item->takeChildren()) delete child;
        item->setData(0, OpenedRole, true);
        item->setIcon(0, Icons::connection(c.type, true));
        loadDatabases(item, c.connId);
        return true;
    }
    return false;  // 树上没有这个连接
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
        item->setIcon(0, Icons::connection(c.type, true));  // 连接成功:点亮
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
