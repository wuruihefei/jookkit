#ifndef JOOKKIT_MAINWINDOW_H
#define JOOKKIT_MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class BackendProcess;
class BackendClient;
class ContentWidget;

// 主窗口:启动时拉起后端;菜单栏对齐 jookdb(文件/编辑/查询/工具/窗口/帮助)。
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString &jarPath, QWidget *parent = nullptr);

private slots:
    void newConnection();
    void saveCurrentQuery();
    void executeSqlFile();
    void about();

private:
    void buildMenus();
    void buildToolBar();

    BackendProcess *proc_;
    BackendClient *client_;
    ContentWidget *content_;
};

#endif
