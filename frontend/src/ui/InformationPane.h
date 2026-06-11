#ifndef JOOKKIT_INFORMATIONPANE_H
#define JOOKKIT_INFORMATIONPANE_H

#include <QTabWidget>
#include <QString>

class QTableWidget;
class QPlainTextEdit;
class BackendClient;

// 信息窗格:选中表时显示「常规」(列/类型/可空/主键)与「DDL」。
class InformationPane : public QTabWidget {
    Q_OBJECT
public:
    explicit InformationPane(BackendClient *client, QWidget *parent = nullptr);
    void showTable(const QString &connId, const QString &db, const QString &table);
    void clearInfo();

private:
    BackendClient *client_;
    QTableWidget *general_;
    QTableWidget *indexes_;
    QPlainTextEdit *ddl_;
};

#endif
