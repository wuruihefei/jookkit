#ifndef JOOKKIT_MAINWINDOW_H
#define JOOKKIT_MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class BackendProcess;
class BackendClient;
class ContentWidget;
class HistoryPane;
class QDockWidget;
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
    void openUserManager();
    void about();
    void rebuildFavMenu();      // 打开收藏菜单时按存储重建
    void favoriteCurrent();     // 收藏左侧树当前选中的表
    void manageFavorites();     // 管理收藏对话框

private:
    void buildToolBar();
    void buildMenus();

    BackendProcess *proc_;
    BackendClient *client_;
    ContentWidget *content_;
    QToolBar *mainTb_ = nullptr;
    QMenu *favMenu_ = nullptr;
    HistoryPane *historyPane_ = nullptr;
    QDockWidget *histDock_ = nullptr;
};

#endif
