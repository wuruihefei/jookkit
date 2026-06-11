#ifndef JOOKKIT_MAINWINDOW_H
#define JOOKKIT_MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class BackendProcess;
class BackendClient;
class ContentWidget;
class QToolBar;

// 主窗口:菜单栏对齐 Navicat(文件/编辑/视图/收藏/工具/窗口/帮助)。
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString &jarPath, QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newConnection();
    void saveCurrentQuery();
    void executeSqlFile();
    void openOptions();
    void openFindReplace();
    void about();

private:
    void buildToolBar();
    void buildMenus();

    BackendProcess *proc_;
    BackendClient *client_;
    ContentWidget *content_;
    QToolBar *mainTb_ = nullptr;
};

#endif
