#ifndef JOOKKIT_QUERYFORM_H
#define JOOKKIT_QUERYFORM_H

#include <QWidget>
#include <QString>

class QTableWidget;
class QLabel;
class SqlEditor;
class BackendClient;
class QJsonObject;

// SQL 查询工作区:编辑器 + 运行 + 结果表格。支持多语句(按分号切分逐条执行)。
class QueryForm : public QWidget {
    Q_OBJECT
public:
    QueryForm(BackendClient *client, const QString &connId,
              const QString &db, QWidget *parent = nullptr);

    void setSql(const QString &sql);
    SqlEditor *editor() const { return editor_; }

public slots:
    void run();         // 运行全部语句
    void runCurrent();  // 运行选中文本(无选中则全部)
    void saveSql();     // 保存编辑器内容到 .sql 文件
    void formatSql();   // 格式化 SQL

private:
    void runText(const QString &sql);
    void showResult(const QJsonObject &data);

    BackendClient *client_;
    QString connId_;
    QString db_;
    SqlEditor *editor_;
    QTableWidget *grid_;
    QLabel *status_;
};

#endif
