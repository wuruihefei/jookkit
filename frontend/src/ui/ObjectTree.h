#ifndef JOOKKIT_OBJECTTREE_H
#define JOOKKIT_OBJECTTREE_H

#include <QTreeWidget>
#include "backend/ConnData.h"

class BackendClient;

// 左侧对象树:连接 → 数据库 → 表。双击表发出 tableActivated。
class ObjectTree : public QTreeWidget {
    Q_OBJECT
public:
    enum NodeType { Conn = 1, Db = 2, Table = 3 };
    enum Role {
        NodeTypeRole = Qt::UserRole + 1,
        ConnIdRole,
        DbRole,
        LoadedRole
    };

    explicit ObjectTree(BackendClient *client, QWidget *parent = nullptr);

    // 打开连接(OPEN_CONNECTION)并把数据库挂为子节点。成功返回 true。
    bool addConnection(const ConnData &c);

    // 当前选中项相关的上下文(无则返回空串)。
    QString currentConnId() const;
    QString currentDb() const;
    QString currentTable() const;

signals:
    void tableActivated(const QString &connId, const QString &db, const QString &table);
    void structureRequested(const QString &connId, const QString &db, const QString &table);

private slots:
    void onItemExpanded(QTreeWidgetItem *item);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void showContextMenu(const QPoint &pos);

private:
    void loadDatabases(QTreeWidgetItem *connItem, const QString &connId);
    void loadTables(QTreeWidgetItem *dbItem, const QString &connId, const QString &db);

    BackendClient *client_;
};

#endif
