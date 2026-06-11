#include "ui/QueryForm.h"
#include "ui/SqlEditor.h"
#include "sql/SqlSplitter.h"
#include "backend/BackendClient.h"
#include "backend/FuncId.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QToolBar>
#include <QVBoxLayout>
#include <QSplitter>
#include <QJsonObject>
#include <QJsonArray>
#include <QShortcut>
#include <QKeySequence>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QTextCursor>

QueryForm::QueryForm(BackendClient *client, const QString &connId,
                     const QString &db, QWidget *parent)
    : QWidget(parent), client_(client), connId_(connId), db_(db) {

    auto *toolbar = new QToolBar;
    toolbar->addAction(tr("运行 (Ctrl+Enter)"), this, &QueryForm::run);
    toolbar->addAction(tr("运行选中"), this, &QueryForm::runCurrent);

    editor_ = new SqlEditor;
    grid_ = new QTableWidget;
    grid_->horizontalHeader()->setStretchLastSection(true);
    status_ = new QLabel(tr("就绪"));

    auto *split = new QSplitter(Qt::Vertical);
    split->addWidget(editor_);
    split->addWidget(grid_);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 2);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(toolbar);
    layout->addWidget(split, 1);
    layout->addWidget(status_);

    auto *sc = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    connect(sc, &QShortcut::activated, this, &QueryForm::run);
}

void QueryForm::setSql(const QString &sql) {
    editor_->setPlainText(sql);
}

void QueryForm::run() {
    runText(editor_->toPlainText());
}

void QueryForm::runCurrent() {
    QString sel = editor_->textCursor().selectedText();
    // QTextCursor 用 U+2029 作段分隔,换回换行
    sel.replace(QChar(0x2029), '\n');
    runText(sel.trimmed().isEmpty() ? editor_->toPlainText() : sel);
}

void QueryForm::saveSql() {
    QString f = QFileDialog::getSaveFileName(this, tr("保存 SQL"), "query.sql",
                                             tr("SQL 文件 (*.sql);;所有文件 (*)"));
    if (f.isEmpty()) return;
    QFile file(f);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&file) << editor_->toPlainText();
        file.close();
        status_->setText(tr("已保存到 %1").arg(f));
    }
}

void QueryForm::runText(const QString &text) {
    if (!client_) return;
    const QStringList stmts = SqlSplitter::split(text);
    if (stmts.isEmpty()) { status_->setText(tr("没有可执行的语句")); return; }

    int totalAffected = 0;
    bool haveQuery = false;
    QJsonObject lastQuery;
    int execCount = 0;

    for (const QString &sql : stmts) {
        QJsonObject req;
        req.insert("funcId", FuncId::EXEC_SQL);
        req.insert("connId", connId_);
        req.insert("sql", sql);
        auto r = client_->call(req);
        if (!r.ok) {
            status_->setText(tr("错误[第%1条]: %2").arg(execCount + 1).arg(r.errorMessage));
            return;
        }
        ++execCount;
        if (r.data.value("isQuery").toBool()) {
            haveQuery = true;
            lastQuery = r.data;
        } else {
            totalAffected += r.data.value("affected").toInt();
        }
    }

    if (haveQuery) {
        showResult(lastQuery);
        status_->setText(tr("执行 %1 条语句;最后查询返回 %2 行")
                         .arg(execCount).arg(lastQuery.value("rows").toArray().size()));
    } else {
        grid_->clear();
        grid_->setRowCount(0);
        grid_->setColumnCount(0);
        status_->setText(tr("执行 %1 条语句;影响行数 %2").arg(execCount).arg(totalAffected));
    }
}

void QueryForm::showResult(const QJsonObject &data) {
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
        for (int j = 0; j < row.size() && j < cols.size(); ++j) {
            const QJsonValue v = row.at(j);
            grid_->setItem(i, j, new QTableWidgetItem(
                v.isNull() ? QStringLiteral("(null)") : v.toVariant().toString()));
        }
    }
}
