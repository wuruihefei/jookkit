#ifndef JOOKKIT_TABLESTRUCTUREFORM_H
#define JOOKKIT_TABLESTRUCTUREFORM_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QHash>

class QTableWidget;
class QLabel;
class QAction;
class BackendClient;

// 表结构视图(可编辑):列名/类型/可空可改,类型下拉选项随数据源(mysql/sqlite)。
// 保存时本地校验后生成 ALTER TABLE,经 EXEC_SQL 执行。
// 主键列只允许改名(类型/可空涉及 AUTO_INCREMENT 等,暂不支持)。
// 连接类型未知时退化为只读视图。
class TableStructureForm : public QWidget {
    Q_OBJECT
public:
    TableStructureForm(BackendClient *client, const QString &connId,
                       const QString &db, const QString &table,
                       const QString &dbType, QWidget *parent = nullptr);

private slots:
    void load();
    void addField();
    void removeField();
    void saveChanges();

private:
    struct ColMeta {          // MySQL information_schema 的完整列定义
        QString columnType;   // 含长度/精度,如 varchar(255)、decimal(12,4)
        bool nullable = true;
        QVariant defaultValue;  // invalid = 无默认值
        QString extra;          // auto_increment / DEFAULT_GENERATED / on update ...
        QString comment;
    };
    struct RowData {
        QString origName;   // 空 = 新增行
        QString name;
        QString type;
        bool nullable = true;
        QString origType;
        bool origNullable = true;
        bool pk = false;
        QVariant origDefault;
        QString origExtra;
        QString origComment;
    };

    bool isMysql() const { return dbType_ == "mysql"; }
    bool isSqlite() const { return dbType_ == "sqlite"; }
    QStringList typeOptions() const;          // 下拉选项,随 dbType_
    QStringList allowedBaseTypes() const;     // 校验用基础类型白名单
    QString quoteIdent(const QString &id) const;
    QString quoteTable() const;
    static QString quoteLiteral(QString s);
    bool fetchMysqlColumnMeta(QHash<QString, ColMeta> &out);  // information_schema
    bool collectRows(QList<RowData> &rows, QString &error) const;
    QStringList buildAlterSql(const QList<RowData> &rows, QString &error) const;
    void setRow(int row, const RowData &rd);

    BackendClient *client_;
    QString connId_;
    QString db_;
    QString table_;
    QString dbType_;       // "mysql" | "sqlite" | 其他(只读)
    bool editable_ = false;
    bool fullMetaOk_ = false;  // MySQL 完整列定义是否拿到(影响 CHANGE 是否保留默认值/注释)
    QTableWidget *grid_;
    QLabel *status_;
    QStringList dropped_;  // 待删除的已有列名
};

#endif
