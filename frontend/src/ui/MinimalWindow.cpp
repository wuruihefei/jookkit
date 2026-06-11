#include "ui/MinimalWindow.h"
#include "backend/ConnData.h"
#include "backend/FuncId.h"

#include <QPlainTextEdit>
#include <QTableWidget>
#include <QListWidget>
#include <QSplitter>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QWidget>

MinimalWindow::MinimalWindow(const QString &jarPath, QWidget *parent)
    : QMainWindow(parent),
      proc_(new BackendProcess(jarPath, this)),
      client_(new BackendClient(this)) {

    if (!proc_->start()) {
        QMessageBox::critical(this, "JookKit", "后端启动失败");
    } else {
        client_->setPort(proc_->port());
    }

    auto *tb = addToolBar("main");
    tb->addAction("打开 SQLite...", this, &MinimalWindow::openSqlite);
    tb->addAction("运行 SQL", this, &MinimalWindow::runSql);

    tableList_ = new QListWidget;
    editor_ = new QPlainTextEdit;
    grid_ = new QTableWidget;

    auto *right = new QWidget;
    auto *rl = new QVBoxLayout(right);
    rl->addWidget(editor_, 1);
    rl->addWidget(grid_, 2);

    auto *split = new QSplitter;
    split->addWidget(tableList_);
    split->addWidget(right);
    split->setStretchFactor(1, 1);
    setCentralWidget(split);
    resize(900, 600);
}

void MinimalWindow::openSqlite() {
    QString file = QFileDialog::getOpenFileName(this, "选择 SQLite 文件");
    if (file.isEmpty()) return;
    ConnData c;
    c.connId = "c1"; c.type = "sqlite"; c.file = file;
    auto r = client_->call(c.toOpenRequest());
    if (!r.ok) { QMessageBox::warning(this, "连接失败", r.errorMessage); return; }
    connected_ = true;
    refreshTables();
}

void MinimalWindow::refreshTables() {
    if (!connected_) return;
    QJsonObject req;
    req.insert("funcId", FuncId::LIST_TABLES);
    req.insert("connId", "c1");
    req.insert("db", "main");
    auto r = client_->call(req);
    tableList_->clear();
    if (!r.ok) return;
    for (const auto &t : r.data.value("tables").toArray())
        tableList_->addItem(t.toString());
}

void MinimalWindow::runSql() {
    if (!connected_) { QMessageBox::information(this, "提示", "请先打开连接"); return; }
    QJsonObject req;
    req.insert("funcId", FuncId::EXEC_SQL);
    req.insert("connId", "c1");
    req.insert("sql", editor_->toPlainText());
    auto r = client_->call(req);
    if (!r.ok) { QMessageBox::warning(this, "SQL 错误", r.errorMessage); return; }

    QJsonObject data = r.data;
    if (!data.value("isQuery").toBool()) {
        QMessageBox::information(this, "完成",
            QString("影响行数: %1").arg(data.value("affected").toInt()));
        return;
    }
    QJsonArray cols = data.value("columns").toArray();
    QJsonArray rows = data.value("rows").toArray();
    grid_->clear();
    grid_->setColumnCount(cols.size());
    QStringList headers;
    for (const auto &c : cols) headers << c.toObject().value("name").toString();
    grid_->setHorizontalHeaderLabels(headers);
    grid_->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        QJsonArray row = rows.at(i).toArray();
        for (int j = 0; j < row.size(); ++j)
            grid_->setItem(i, j, new QTableWidgetItem(
                row.at(j).isNull() ? "(null)" : row.at(j).toVariant().toString()));
    }
}
