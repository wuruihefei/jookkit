#ifndef JOOKKIT_TABLESTRUCTUREFORM_H
#define JOOKKIT_TABLESTRUCTUREFORM_H

#include <QWidget>
#include <QString>

class QTableWidget;
class BackendClient;

// 只读表结构视图:列名、类型、可空、主键标记(来自 DESCRIBE_TABLE)。
class TableStructureForm : public QWidget {
    Q_OBJECT
public:
    TableStructureForm(BackendClient *client, const QString &connId,
                       const QString &db, const QString &table,
                       QWidget *parent = nullptr);

private:
    void load();

    BackendClient *client_;
    QString connId_;
    QString db_;
    QString table_;
    QTableWidget *grid_;
};

#endif
