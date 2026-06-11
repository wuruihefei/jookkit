#ifndef JOOKKIT_MINIMALWINDOW_H
#define JOOKKIT_MINIMALWINDOW_H

#include <QMainWindow>
#include "backend/BackendProcess.h"
#include "backend/BackendClient.h"

class QPlainTextEdit;
class QTableWidget;
class QListWidget;

class MinimalWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MinimalWindow(const QString &jarPath, QWidget *parent = nullptr);

private slots:
    void openSqlite();
    void runSql();
    void refreshTables();

private:
    BackendProcess *proc_;
    BackendClient *client_;
    QListWidget *tableList_;
    QPlainTextEdit *editor_;
    QTableWidget *grid_;
    bool connected_ = false;
};

#endif
