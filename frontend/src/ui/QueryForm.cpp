#include "ui/QueryForm.h"
#include "ui/SqlEditor.h"
#include "ui/Icons.h"
#include "sql/SqlSplitter.h"
#include "sql/SqlFormat.h"
#include "backend/BackendClient.h"
#include "backend/FuncId.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QToolBar>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QJsonObject>
#include <QJsonArray>
#include <QShortcut>
#include <QKeySequence>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QTextCursor>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QPlainTextEdit>
#include <QAction>
#include "ui/GridUtils.h"
#include "export/Exporter.h"

QueryForm::QueryForm(BackendClient *client, const QList<ConnData> &conns,
                     const QString &initialConnId, const QString &initialDb, QWidget *parent)
    : QWidget(parent), client_(client), conns_(conns) {

    // 顶部:连接 + 数据库 下拉 + 运行
    connCombo_ = new QComboBox;
    for (const ConnData &c : conns_)
        connCombo_->addItem(c.connId, c.connId);
    int ci = connCombo_->findData(initialConnId);
    if (ci >= 0) connCombo_->setCurrentIndex(ci);

    dbCombo_ = new QComboBox;
    dbCombo_->setMinimumWidth(160);

    auto *topBar = new QToolBar;
    topBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    topBar->addWidget(connCombo_);
    topBar->addWidget(dbCombo_);
    topBar->addSeparator();
    topBar->addAction(Icons::run(), tr("运行"), this, &QueryForm::run);
    topBar->addAction(tr("运行选中"), this, &QueryForm::runCurrent);
    topBar->addAction(tr("格式化"), this, &QueryForm::formatSql);
    topBar->addAction(Icons::save(), tr("保存"), this, &QueryForm::saveSql);

    editor_ = new SqlEditor;
    grid_ = new QTableWidget;
    grid_->horizontalHeader()->setStretchLastSection(true);
    grid_->setSortingEnabled(true);
    status_ = new QLabel(tr("就绪"));

    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("筛选当前结果(仅当前已加载数据)…"));
    connect(filterEdit_, &QLineEdit::textChanged, this, &QueryForm::applyGridFilter);

    // Copy selection as TSV (Ctrl+C)
    auto *copyAct = new QAction(this);
    copyAct->setShortcut(QKeySequence::Copy);
    connect(copyAct, &QAction::triggered, this, &QueryForm::copySelection);
    grid_->addAction(copyAct);

    // Right-click context menu on grid
    grid_->setContextMenuPolicy(Qt::ActionsContextMenu);
    auto *expAct = new QAction(tr("导出…"), this);
    connect(expAct, &QAction::triggered, this, &QueryForm::exportResult);
    grid_->addAction(expAct);

    // Double-click: show full cell value
    connect(grid_, &QTableWidget::cellDoubleClicked, this, &QueryForm::showCellValue);

    auto *split = new QSplitter(Qt::Vertical);
    split->addWidget(editor_);
    split->addWidget(grid_);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 2);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(topBar);
    layout->addWidget(split, 1);
    layout->addWidget(filterEdit_);
    layout->addWidget(status_);

    connect(connCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QueryForm::onConnChanged);
    connect(dbCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ refreshCompletion(); });
    auto *sc = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    connect(sc, &QShortcut::activated, this, &QueryForm::run);

    reloadDbList();
    int di = dbCombo_->findText(initialDb);
    if (di >= 0) dbCombo_->setCurrentIndex(di);
}

ConnData QueryForm::currentConn() const {
    QString id = connCombo_->currentData().toString();
    for (const ConnData &c : conns_)
        if (c.connId == id) return c;
    return ConnData();
}

void QueryForm::onConnChanged(int) {
    reloadDbList();
}

void QueryForm::reloadDbList() {
    ConnData c = currentConn();
    if (c.connId.isEmpty()) return;

    QJsonObject req;
    req.insert("funcId", FuncId::LIST_DATABASES);
    req.insert("connId", c.connId);
    auto r = client_->call(req);
    if (!r.ok && r.errorCode == "NO_CONN") {       // 懒打开
        client_->call(c.toOpenRequest(), 10000);
        r = client_->call(req);
    }
    {
        // 重灌列表期间屏蔽信号,避免 clear/addItem 触发多次补全刷新
        QSignalBlocker block(dbCombo_);
        dbCombo_->clear();
        if (r.ok) {
            for (const auto &d : r.data.value("databases").toArray())
                dbCombo_->addItem(d.toString());
        }
    }
    refreshCompletion();
}

void QueryForm::refreshCompletion() {
    ConnData c = currentConn();
    const QString db = dbCombo_->currentText();
    if (c.connId.isEmpty() || db.isEmpty()) return;
    QJsonObject lt;
    lt.insert("funcId", FuncId::LIST_TABLES);
    lt.insert("connId", c.connId);
    lt.insert("db", db);
    auto r = client_->call(lt);
    if (!r.ok) return;
    QStringList tables;
    for (const auto &t : r.data.value("tables").toArray()) tables << t.toString();
    editor_->setCompletionWords(tables);
}

void QueryForm::setSql(const QString &sql) { editor_->setPlainText(sql); }

void QueryForm::formatSql() { editor_->setPlainText(SqlFormat::format(editor_->toPlainText())); }

void QueryForm::run() { runText(editor_->toPlainText()); }

void QueryForm::runCurrent() {
    QString sel = editor_->textCursor().selectedText();
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
    ConnData c = currentConn();
    if (c.connId.isEmpty()) { status_->setText(tr("请先选择连接")); return; }
    const QString connId = c.connId;
    const QString db = dbCombo_->currentText();

    // MySQL:切到所选库
    if (c.type == "mysql" && !db.isEmpty()) {
        QJsonObject use;
        use.insert("funcId", FuncId::EXEC_SQL);
        use.insert("connId", connId);
        use.insert("sql", QString("USE `%1`").arg(db));
        auto ur = client_->call(use);
        if (!ur.ok && ur.errorCode == "NO_CONN") {
            client_->call(c.toOpenRequest(), 10000);
            client_->call(use);
        }
    }

    const QStringList stmts = SqlSplitter::split(text);
    if (stmts.isEmpty()) { status_->setText(tr("没有可执行的语句")); return; }

    int totalAffected = 0;
    bool haveQuery = false;
    QJsonObject lastQuery;
    int execCount = 0;

    for (const QString &sql : stmts) {
        QJsonObject req;
        req.insert("funcId", FuncId::EXEC_SQL);
        req.insert("connId", connId);
        req.insert("sql", sql);
        client_->setHistoryContext(connId, db);
        auto r = client_->call(req);
        if (!r.ok) {
            status_->setText(tr("错误[第%1条]: %2").arg(execCount + 1).arg(r.errorMessage));
            return;
        }
        ++execCount;
        if (r.data.value("isQuery").toBool()) { haveQuery = true; lastQuery = r.data; }
        else totalAffected += r.data.value("affected").toInt();
    }

    if (haveQuery) {
        showResult(lastQuery);
        status_->setText(tr("执行 %1 条;最后查询返回 %2 行")
                         .arg(execCount).arg(lastQuery.value("rows").toArray().size()));
    } else {
        grid_->clear();
        grid_->setRowCount(0);
        grid_->setColumnCount(0);
        status_->setText(tr("执行 %1 条;影响行数 %2").arg(execCount).arg(totalAffected));
    }
}

void QueryForm::showResult(const QJsonObject &data) {
    QJsonArray cols = data.value("columns").toArray();
    QJsonArray rows = data.value("rows").toArray();
    grid_->setSortingEnabled(false);
    grid_->clear();
    if (filterEdit_) filterEdit_->clear();
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
    grid_->setSortingEnabled(true);
}

void QueryForm::applyGridFilter(const QString &text) {
    for (int i = 0; i < grid_->rowCount(); ++i) {
        bool match = text.isEmpty();
        for (int j = 0; !match && j < grid_->columnCount(); ++j) {
            auto *it = grid_->item(i, j);
            if (it && it->text().contains(text, Qt::CaseInsensitive)) match = true;
        }
        grid_->setRowHidden(i, !match);
    }
}

void QueryForm::copySelection() {
    const bool hasSelection = !grid_->selectedRanges().isEmpty();
    QStringList headers; QList<QStringList> rows;
    GridUtils::extract(grid_, true, hasSelection, headers, rows);
    QByteArray tsv = Exporter::toCsv(headers, rows, '\t', true, false);
    QApplication::clipboard()->setText(QString::fromUtf8(tsv));
}

void QueryForm::exportResult() {
    if (grid_->rowCount() == 0) {
        QMessageBox::information(this, tr("导出"), tr("没有可导出的数据。"));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("导出结果"), QString(),
        "CSV (*.csv);;JSON (*.json);;SQL INSERT (*.sql)");
    if (path.isEmpty()) return;

    const bool hasSelection = !grid_->selectedRanges().isEmpty();
    QStringList headers; QList<QStringList> rows;
    GridUtils::extract(grid_, true, hasSelection, headers, rows);

    QByteArray bytes;
    if (path.endsWith(".json", Qt::CaseInsensitive))
        bytes = Exporter::toJson(headers, rows);
    else if (path.endsWith(".sql", Qt::CaseInsensitive))
        bytes = Exporter::toInsertSql("exported_table", headers, rows, QString(), true);
    else
        bytes = Exporter::toCsv(headers, rows, ',', true, true);  // UTF-8 BOM for Excel

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("导出"), tr("无法写入文件:\n") + path);
        return;
    }
    f.write(bytes);
    QMessageBox::information(this, tr("导出"),
        QString(tr("已导出 %1 行到\n%2")).arg(rows.size()).arg(path));
}

void QueryForm::showCellValue(int row, int col) {
    auto *it = grid_->item(row, col);
    if (!it) return;
    QDialog dlg(this);
    dlg.setWindowTitle(tr("单元格内容"));
    dlg.resize(500, 360);
    auto *lay = new QVBoxLayout(&dlg);
    auto *edit = new QPlainTextEdit(&dlg);
    edit->setReadOnly(true);
    edit->setPlainText(it->text());
    lay->addWidget(edit);
    dlg.exec();
}
