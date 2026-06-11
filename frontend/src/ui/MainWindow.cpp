#include "ui/MainWindow.h"
#include "ui/ContentWidget.h"
#include "ui/ConnDialog.h"
#include "ui/QueryForm.h"
#include "ui/SqlEditor.h"
#include "backend/BackendProcess.h"
#include "backend/BackendClient.h"

#include <QToolBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QPlainTextEdit>
#include <QStyle>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QStatusBar>

MainWindow::MainWindow(const QString &jarPath, QWidget *parent)
    : QMainWindow(parent),
      proc_(new BackendProcess(jarPath, this)),
      client_(new BackendClient(this)) {

    setWindowTitle("JookKit");
    resize(1040, 700);

    if (!proc_->start()) {
        QMessageBox::critical(this, "JookKit",
            tr("后端启动失败,请确认 Java 可用且 jar 路径正确:\n%1").arg(jarPath));
    } else {
        client_->setPort(proc_->port());
    }

    content_ = new ContentWidget(client_);
    setCentralWidget(content_);

    buildMenus();
    buildToolBar();

    statusBar()->showMessage(proc_->isRunning()
        ? tr("后端已就绪 (端口 %1)").arg(proc_->port())
        : tr("后端未启动"));
}

void MainWindow::buildToolBar() {
    QStyle *st = style();
    auto *tb = addToolBar(tr("主工具栏"));
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tb->addAction(st->standardIcon(QStyle::SP_DriveNetIcon), tr("新建数据源"),
                  this, &MainWindow::newConnection);
    tb->addAction(st->standardIcon(QStyle::SP_FileIcon), tr("新建查询"),
                  content_, &ContentWidget::newQuery);
    tb->addAction(st->standardIcon(QStyle::SP_MediaPlay), tr("运行"), this, [this]{
        if (auto *q = content_->currentQueryForm()) q->run();
    });
    tb->addSeparator();
    tb->addAction(st->standardIcon(QStyle::SP_DialogSaveButton), tr("保存"),
                  this, &MainWindow::saveCurrentQuery);
}

void MainWindow::buildMenus() {
    // 路由编辑操作到当前焦点控件(QPlainTextEdit/QLineEdit 自带这些槽)
    auto routeEdit = [](const char *slot) {
        if (QWidget *w = QApplication::focusWidget())
            QMetaObject::invokeMethod(w, slot);
    };

    // ---- 文件 ----
    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(tr("新建数据源"), this, &MainWindow::newConnection);
    fileMenu->addAction(tr("新建查询"), content_, &ContentWidget::newQuery);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("另存为..."), this, &MainWindow::saveCurrentQuery,
                        QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), this, &QWidget::close);

    // ---- 编辑 ----
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
    QAction *replaceAct = editMenu->addAction(tr("替换..."));
    replaceAct->setEnabled(false);
    replaceAct->setToolTip(tr("暂未实现"));

    // ---- 查询 ----
    QMenu *queryMenu = menuBar()->addMenu(tr("查询(&Q)"));
    queryMenu->addAction(tr("运行脚本"), this, [this]{
        if (auto *q = content_->currentQueryForm()) q->run();
    }, QKeySequence(tr("Ctrl+Return")));
    queryMenu->addAction(tr("运行当前语句"), this, [this]{
        if (auto *q = content_->currentQueryForm()) q->runCurrent();
    });
    QAction *ccAct = queryMenu->addAction(tr("代码补全"));
    ccAct->setEnabled(false); ccAct->setToolTip(tr("暂未实现"));

    // ---- 工具 ----
    QMenu *toolMenu = menuBar()->addMenu(tr("工具(&T)"));
    for (const QString &t : {tr("数据库管理"), tr("对象管理"),
                             tr("数据同步..."), tr("结构同步..."), tr("生成数据库文档...")}) {
        QAction *a = toolMenu->addAction(t);
        a->setEnabled(false); a->setToolTip(tr("暂未实现"));
    }
    toolMenu->addSeparator();
    toolMenu->addAction(tr("执行 SQL 文件..."), this, &MainWindow::executeSqlFile);
    QAction *prefAct = toolMenu->addAction(tr("偏好设置..."));
    prefAct->setEnabled(false); prefAct->setToolTip(tr("暂未实现"));

    // ---- 窗口 ----
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
    winMenu->addSeparator();
    winMenu->addAction(tr("折叠侧栏"), content_, &ContentWidget::toggleSidebar,
                       QKeySequence(tr("Ctrl+B")));

    // ---- 帮助 ----
    QMenu *helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
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
    if (!content_->addConnection(c))
        QMessageBox::warning(this, tr("新建连接"), tr("打开连接失败"));
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
