#include "ui/MainWindow.h"
#include "ui/ContentWidget.h"
#include "ui/ConnDialog.h"
#include "ui/QueryForm.h"
#include "ui/SqlEditor.h"
#include "ui/Icons.h"
#include "ui/OptionsDialog.h"
#include "ui/FindReplaceDialog.h"
#include "ui/UserManagerDialog.h"
#include "store/ConnectionStore.h"
#include "backend/BackendProcess.h"
#include "backend/BackendClient.h"

#include <QToolBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QStatusBar>
#include <QKeySequence>
#include <QCloseEvent>

MainWindow::MainWindow(const QString &jarPath, QWidget *parent)
    : QMainWindow(parent),
      proc_(new BackendProcess(jarPath, this)),
      client_(new BackendClient(this)) {

    setWindowTitle("JookKit");
    resize(1080, 720);

    if (!proc_->start()) {
        QMessageBox::critical(this, "JookKit",
            tr("后端启动失败,请确认 Java 可用且 jar 路径正确:\n%1").arg(jarPath));
    } else {
        client_->setPort(proc_->port());
    }

    content_ = new ContentWidget(client_);
    setCentralWidget(content_);

    buildToolBar();
    buildMenus();

    statusBar()->showMessage(proc_->isRunning()
        ? tr("后端已就绪 (端口 %1)").arg(proc_->port())
        : tr("后端未启动"));

    // 加载已保存的连接(懒打开)
    for (const ConnData &c : ConnectionStore::load())
        content_->addSavedConnection(c);
}

void MainWindow::buildToolBar() {
    mainTb_ = addToolBar(tr("主工具栏"));
    mainTb_->setObjectName("mainToolBar");
    mainTb_->setMovable(false);
    mainTb_->setIconSize(QSize(28, 28));
    mainTb_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mainTb_->addAction(Icons::connection(), tr("新建连接"), this, &MainWindow::newConnection);
    mainTb_->addAction(Icons::query(), tr("新建查询"), content_, &ContentWidget::newQuery);
    mainTb_->addAction(Icons::run(), tr("运行"), this, [this]{
        if (auto *q = content_->currentQueryForm()) q->run();
    });
    mainTb_->addSeparator();
    mainTb_->addAction(Icons::data(), tr("打开数据"), content_, &ContentWidget::viewCurrentData);
    mainTb_->addAction(Icons::structure(), tr("查看结构"), content_, &ContentWidget::viewCurrentStructure);
}

void MainWindow::buildMenus() {
    auto routeEdit = [](const char *slot) {
        if (QWidget *w = QApplication::focusWidget())
            QMetaObject::invokeMethod(w, slot);
    };
    auto disabled = [](QMenu *m, const QString &text) {
        QAction *a = m->addAction(text);
        a->setEnabled(false);
        a->setToolTip(QObject::tr("暂未实现"));
    };

    // 文件
    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(Icons::connection(), tr("新建连接"), this, &MainWindow::newConnection);
    fileMenu->addAction(Icons::query(), tr("新建查询"), content_, &ContentWidget::newQuery);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("打开 SQL 文件..."), this, &MainWindow::executeSqlFile);
    fileMenu->addAction(tr("另存为..."), this, &MainWindow::saveCurrentQuery, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), this, &QWidget::close);

    // 编辑
    QMenu *editMenu = menuBar()->addMenu(tr("编辑(&E)"));
    editMenu->addAction(tr("撤销"), this, [=]{ routeEdit("undo"); }, QKeySequence::Undo);
    editMenu->addAction(tr("重做"), this, [=]{ routeEdit("redo"); }, QKeySequence::Redo);
    editMenu->addSeparator();
    editMenu->addAction(tr("剪切"), this, [=]{ routeEdit("cut"); }, QKeySequence::Cut);
    editMenu->addAction(tr("复制"), this, [=]{ routeEdit("copy"); }, QKeySequence::Copy);
    editMenu->addAction(tr("粘贴"), this, [=]{ routeEdit("paste"); }, QKeySequence::Paste);
    editMenu->addAction(tr("全选"), this, [=]{ routeEdit("selectAll"); }, QKeySequence::SelectAll);
    editMenu->addSeparator();
    editMenu->addAction(tr("行注释"), this, [=]{
        if (auto *e = qobject_cast<SqlEditor *>(QApplication::focusWidget())) e->toggleComment();
    }, QKeySequence(tr("Ctrl+/")));
    editMenu->addAction(tr("格式化 SQL"), this, [this]{
        if (auto *q = content_->currentQueryForm()) q->formatSql();
    }, QKeySequence(tr("Ctrl+Shift+F")));
    editMenu->addAction(tr("查找替换..."), this, &MainWindow::openFindReplace, QKeySequence::Find);

    // 视图
    QMenu *viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    QAction *navAct = viewMenu->addAction(tr("导航窗格"));
    navAct->setCheckable(true); navAct->setChecked(true);
    connect(navAct, &QAction::toggled, content_, &ContentWidget::setSidebarVisible);
    QAction *infoAct = viewMenu->addAction(tr("信息窗格"));
    infoAct->setCheckable(true); infoAct->setChecked(false);  // 默认最小化
    connect(infoAct, &QAction::toggled, content_, &ContentWidget::setInfoVisible);
    viewMenu->addSeparator();
    QAction *tbAct = viewMenu->addAction(tr("主工具栏"));
    tbAct->setCheckable(true); tbAct->setChecked(true);
    connect(tbAct, &QAction::toggled, mainTb_, &QToolBar::setVisible);

    // 收藏
    QMenu *favMenu = menuBar()->addMenu(tr("收藏(&A)"));
    disabled(favMenu, tr("收藏当前"));
    disabled(favMenu, tr("管理收藏..."));

    // 工具(数据传输/数据同步/结构同步等未实现功能暂不展示)
    QMenu *toolMenu = menuBar()->addMenu(tr("工具(&T)"));
    toolMenu->addAction(tr("执行 SQL 文件..."), this, &MainWindow::executeSqlFile);
    toolMenu->addAction(tr("用户管理..."), this, &MainWindow::openUserManager);
    toolMenu->addSeparator();
    toolMenu->addAction(tr("选项..."), this, &MainWindow::openOptions);

    // 窗口
    QMenu *winMenu = menuBar()->addMenu(tr("窗口(&W)"));
    winMenu->addAction(tr("上一个标签"), content_, &ContentWidget::previousTab,
                       QKeySequence(tr("Ctrl+Shift+Tab")));
    winMenu->addAction(tr("下一个标签"), content_, &ContentWidget::nextTab,
                       QKeySequence(tr("Ctrl+Tab")));
    winMenu->addSeparator();
    winMenu->addAction(tr("关闭标签"), content_, &ContentWidget::closeCurrentTab,
                       QKeySequence(tr("Ctrl+W")));
    winMenu->addAction(tr("关闭其他"), content_, &ContentWidget::closeOtherTabs);
    winMenu->addAction(tr("关闭全部"), content_, &ContentWidget::closeAllTabs);

    // 帮助
    QMenu *helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    disabled(helpMenu, tr("在线文档"));
    helpMenu->addAction(tr("关于"), this, &MainWindow::about);
}

void MainWindow::newConnection() {
    ConnDialog dlg(client_, this);
    if (dlg.exec() != QDialog::Accepted) return;
    ConnData c = dlg.connData();
    if (c.connId.isEmpty()) {
        QMessageBox::warning(this, tr("新建连接"), tr("连接名不能为空"));
        return;
    }
    if (!content_->addConnection(c)) {
        QMessageBox::warning(this, tr("新建连接"), tr("打开连接失败"));
        return;
    }
    ConnectionStore::save(content_->allConnections());  // 持久化
    statusBar()->showMessage(tr("连接已保存"), 3000);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // 关闭时再保存一次,确保任何连接变更落盘
    if (content_) ConnectionStore::save(content_->allConnections());
    // 立即停掉后端子进程(在事件循环仍在时执行,可靠等待/强杀),避免遗留进程
    if (proc_) proc_->stop();
    event->accept();
    QMainWindow::closeEvent(event);
}

void MainWindow::openOptions() {
    OptionsDialog(this).exec();
}

void MainWindow::openUserManager() {
    ConnData c = content_->currentConnData();
    if (c.connId.isEmpty()) {
        QMessageBox::information(this, tr("用户管理"), tr("请先在左侧选择一个连接"));
        return;
    }
    if (c.type != "mysql") {
        QMessageBox::information(this, tr("用户管理"), tr("仅 MySQL 连接支持用户管理"));
        return;
    }
    UserManagerDialog dlg(client_, c.connId, this);
    dlg.exec();
}

void MainWindow::openFindReplace() {
    QPlainTextEdit *ed = qobject_cast<QPlainTextEdit *>(QApplication::focusWidget());
    if (!ed) {
        if (auto *q = content_->currentQueryForm()) ed = q->editor();
    }
    if (!ed) {
        QMessageBox::information(this, tr("查找替换"), tr("请先打开一个查询"));
        return;
    }
    auto *d = new FindReplaceDialog(ed, this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

void MainWindow::saveCurrentQuery() {
    if (auto *q = content_->currentQueryForm()) q->saveSql();
    else QMessageBox::information(this, tr("另存为"), tr("当前不是查询标签"));
}

void MainWindow::executeSqlFile() {
    QString f = QFileDialog::getOpenFileName(this, tr("执行 SQL 文件"), QString(),
                                             tr("SQL 文件 (*.sql);;所有文件 (*)"));
    if (f.isEmpty()) return;
    QFile file(f);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString sql = QTextStream(&file).readAll();
    file.close();
    content_->newQuery();
    if (auto *q = content_->currentQueryForm()) { q->setSql(sql); q->run(); }
}

void MainWindow::about() {
    QMessageBox::about(this, tr("关于 JookKit"),
        tr("<b>JookKit</b><br>轻量数据库工具(C++/Qt 前端 + Java 后端)。<br>"
           "支持 MySQL/MariaDB/SQLite。仅供测试验证。"));
}
