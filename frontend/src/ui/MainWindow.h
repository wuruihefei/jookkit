#ifndef JOOKKIT_MAINWINDOW_H
#define JOOKKIT_MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class BackendProcess;
class BackendClient;
class ContentWidget;

// 主窗口:启动时拉起后端,菜单/工具栏驱动连接与查询。
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString &jarPath, QWidget *parent = nullptr);

private slots:
    void newConnection();

private:
    BackendProcess *proc_;
    BackendClient *client_;
    ContentWidget *content_;
};

#endif
