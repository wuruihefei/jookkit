#include "ui/MainWindow.h"
#include "ui/ContentWidget.h"
#include "ui/ConnDialog.h"
#include "backend/BackendProcess.h"
#include "backend/BackendClient.h"

#include <QToolBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(const QString &jarPath, QWidget *parent)
    : QMainWindow(parent),
      proc_(new BackendProcess(jarPath, this)),
      client_(new BackendClient(this)) {

    setWindowTitle("JookKit");
    resize(1000, 680);

    if (!proc_->start()) {
        QMessageBox::critical(this, "JookKit",
            tr("后端启动失败,请确认 Java 已安装且 jar 路径正确:\n%1").arg(jarPath));
    } else {
        client_->setPort(proc_->port());
    }

    content_ = new ContentWidget(client_);
    setCentralWidget(content_);

    auto *tb = addToolBar(tr("主工具栏"));
    tb->addAction(tr("新建连接"), this, &MainWindow::newConnection);
    tb->addAction(tr("新建查询"), content_, &ContentWidget::newQuery);
    tb->addSeparator();
    tb->addAction(tr("查看数据"), content_, &ContentWidget::viewCurrentData);
    tb->addAction(tr("查看结构"), content_, &ContentWidget::viewCurrentStructure);

    auto *fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(tr("新建连接"), this, &MainWindow::newConnection);
    fileMenu->addAction(tr("新建查询"), content_, &ContentWidget::newQuery);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), this, &QWidget::close);

    statusBar()->showMessage(proc_->isRunning()
        ? tr("后端已就绪 (端口 %1)").arg(proc_->port())
        : tr("后端未启动"));
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
