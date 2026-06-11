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

public slots:
    void run();

private:
    void showResult(const QJsonObject &data);

    BackendClient *client_;
    QString connId_;
    QString db_;
    SqlEditor *editor_;
    QTableWidget *grid_;
    QLabel *status_;
};

#endif
