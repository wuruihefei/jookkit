#ifndef JOOKKIT_QUERYFORM_H
#define JOOKKIT_QUERYFORM_H

#include <QWidget>
#include <QString>
#include <QList>
#include "backend/ConnData.h"

class QTableWidget;
class QLabel;
class QComboBox;
class QLineEdit;
class SqlEditor;
class BackendClient;
class QJsonObject;

// SQL 查询工作区:顶部「连接 + 数据库」下拉(可切换目标),编辑器 + 运行 + 结果表格。
class QueryForm : public QWidget {
    Q_OBJECT
public:
    QueryForm(BackendClient *client, const QList<ConnData> &conns,
              const QString &initialConnId, const QString &initialDb,
              QWidget *parent = nullptr);

    void setSql(const QString &sql);
    SqlEditor *editor() const { return editor_; }

public slots:
    void run();
    void runCurrent();
    void saveSql();
    void formatSql();

private slots:
    void onConnChanged(int index);
    void applyGridFilter(const QString &text);
    void exportResult();
    void copySelection();
    void showCellValue(int row, int col);

private:
    ConnData currentConn() const;
    void reloadDbList();
    void refreshCompletion();  // 按当前所选库刷新表名补全词
    void runText(const QString &sql);
    void showResult(const QJsonObject &data);

    BackendClient *client_;
    QList<ConnData> conns_;
    QComboBox *connCombo_;
    QComboBox *dbCombo_;
    SqlEditor *editor_;
    QTableWidget *grid_;
    QLineEdit *filterEdit_ = nullptr;
    QLabel *status_;
};

#endif
