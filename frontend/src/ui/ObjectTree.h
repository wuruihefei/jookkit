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
        LoadedRole,
        ConnDataRole,
        OpenedRole
    };

    explicit ObjectTree(BackendClient *client, QWidget *parent = nullptr);

    // 打开连接(OPEN_CONNECTION)并把数据库挂为子节点。成功返回 true。
    bool addConnection(const ConnData &c);
    // 添加已保存连接(不立即连接,展开时才打开)。
    void addSavedConnection(const ConnData &c);
    // 当前所有连接配置(用于持久化)。
    QList<ConnData> allConnections() const;

    // 确保某连接已打开(收藏直达等场景):未打开则按保存的配置连接并加载库列表。
    bool ensureOpen(const QString &connId);

    // 当前选中项相关的上下文(无则返回空串)。
    QString currentConnId() const;
    QString currentDb() const;
    QString currentTable() const;

signals:
    void tableActivated(const QString &connId, const QString &db, const QString &table);
    void structureRequested(const QString &connId, const QString &db, const QString &table);
    void dataImportRequested(const QString &connId, const QString &db, const QString &table);
    void connectionsChanged();  // 编辑/复制/删除连接后发出,用于持久化

private slots:
    void onItemExpanded(QTreeWidgetItem *item);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void showContextMenu(const QPoint &pos);

private:
    void loadDatabases(QTreeWidgetItem *connItem, const QString &connId);
    void loadTables(QTreeWidgetItem *dbItem, const QString &connId, const QString &db);
    void editConnection(QTreeWidgetItem *item);
    void closeConnection(QTreeWidgetItem *item);    // 断开:保留配置,节点重置为未连接
    void resetToClosed(QTreeWidgetItem *item, const ConnData &c);  // 节点置为未连接态
    void duplicateConnection(QTreeWidgetItem *item);
    void removeConnection(QTreeWidgetItem *item);
    void closeBackendConn(const QString &connId);   // 已打开则通知后端关闭
    QString uniqueConnId(const QString &base) const;
    bool connIdExists(const QString &id, const QTreeWidgetItem *except = nullptr) const;

    BackendClient *client_;
};

#endif
