#include "ui/ImportDialog.h"
#include "backend/BackendClient.h"
#include "backend/FuncId.h"
#include "import/CsvReader.h"
#include "import/JsonReader.h"
#include "import/ColumnMapping.h"
#include "import/ImportRunner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QProgressDialog>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>

namespace {
const int kPreviewRows = 50;
const int kBatchSize   = 200;
const QString kNullText  = QStringLiteral("<NULL>");
const QString kSkipLabel = QStringLiteral("(不导入)");

// 单元格渲染:null → 灰色 <NULL>;其它原样。
QTableWidgetItem *cellItem(const QString &v) {
    auto *it = new QTableWidgetItem(v.isNull() ? kNullText : v);
    if (v.isNull()) it->setForeground(Qt::gray);
    it->setFlags(it->flags() & ~Qt::ItemIsEditable);
    return it;
}
}

ImportDialog::ImportDialog(BackendClient *client, const QString &connId, const QString &db,
                           const QString &table, const QString &dbType, QWidget *parent)
    : QDialog(parent), client_(client), connId_(connId), db_(db),
      table_(table), dbType_(dbType) {
    setWindowTitle(tr("导入数据到表 %1").arg(table_));
    resize(720, 640);

    auto *root = new QVBoxLayout(this);

    // —— 文件与解析选项 ——
    auto *optBox = new QGroupBox(tr("数据源"));
    auto *form = new QFormLayout(optBox);

    auto *pathRow = new QHBoxLayout;
    pathEdit_ = new QLineEdit;
    pathEdit_->setReadOnly(true);
    auto *browseBtn = new QPushButton(tr("选择文件..."));
    pathRow->addWidget(pathEdit_, 1);
    pathRow->addWidget(browseBtn);
    form->addRow(tr("文件"), pathRow);

    encodingCombo_ = new QComboBox;
    encodingCombo_->addItem(tr("自动检测"), QString());
    encodingCombo_->addItem("UTF-8", QStringLiteral("UTF-8"));
    encodingCombo_->addItem("GBK", QStringLiteral("GBK"));
    encodingCombo_->addItem("GB18030", QStringLiteral("GB18030"));
    form->addRow(tr("编码"), encodingCombo_);

    sepCombo_ = new QComboBox;
    sepCombo_->addItem(tr("逗号 ,"), QStringLiteral(","));
    sepCombo_->addItem(tr("制表符 Tab"), QStringLiteral("\t"));
    sepCombo_->addItem(tr("分号 ;"), QStringLiteral(";"));
    form->addRow(tr("分隔符 (CSV)"), sepCombo_);

    headerCheck_ = new QCheckBox(tr("首行为列名"));
    headerCheck_->setChecked(true);
    form->addRow(QString(), headerCheck_);

    infoLabel_ = new QLabel;
    infoLabel_->setStyleSheet("color: gray;");
    form->addRow(QString(), infoLabel_);

    root->addWidget(optBox);

    // —— 预览 ——
    root->addWidget(new QLabel(tr("预览(前 %1 行):").arg(kPreviewRows)));
    preview_ = new QTableWidget;
    preview_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    preview_->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(preview_, 1);

    // —— 列映射 ——
    root->addWidget(new QLabel(tr("列映射(目标表列 ← 文件字段):")));
    mapping_ = new QTableWidget(0, 2);
    mapping_->setHorizontalHeaderLabels({tr("目标列"), tr("文件字段")});
    mapping_->horizontalHeader()->setStretchLastSection(true);
    mapping_->verticalHeader()->setVisible(false);
    root->addWidget(mapping_, 1);

    // —— 按钮 ——
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch(1);
    importBtn_ = new QPushButton(tr("开始导入"));
    importBtn_->setEnabled(false);
    auto *cancelBtn = new QPushButton(tr("关闭"));
    btnRow->addWidget(importBtn_);
    btnRow->addWidget(cancelBtn);
    root->addLayout(btnRow);

    connect(browseBtn, &QPushButton::clicked, this, &ImportDialog::browseFile);
    connect(encodingCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ImportDialog::reparse);
    connect(sepCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ImportDialog::reparse);
    connect(headerCheck_, &QCheckBox::toggled, this, &ImportDialog::reparse);
    connect(importBtn_, &QPushButton::clicked, this, &ImportDialog::doImport);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    loadTableColumns();
}

bool ImportDialog::isJson() const {
    return pathEdit_->text().endsWith(".json", Qt::CaseInsensitive);
}

void ImportDialog::loadTableColumns() {
    tableCols_.clear();
    QJsonObject req;
    req.insert("funcId", FuncId::DESCRIBE_TABLE);
    req.insert("connId", connId_);
    req.insert("db", db_);
    req.insert("table", table_);
    auto r = client_->call(req);
    if (!r.ok) {
        QMessageBox::warning(this, tr("导入"),
                             tr("无法获取表结构: %1").arg(r.errorMessage));
        return;
    }
    for (const auto &c : r.data.value("columns").toArray())
        tableCols_ << c.toObject().value("name").toString();
}

void ImportDialog::browseFile() {
    const QString fn = QFileDialog::getOpenFileName(
        this, tr("选择数据文件"), QString(),
        tr("数据文件 (*.csv *.json);;CSV (*.csv);;JSON (*.json);;所有文件 (*)"));
    if (fn.isEmpty()) return;
    pathEdit_->setText(fn);
    // JSON 无需编码/分隔符
    encodingCombo_->setEnabled(!isJson());
    sepCombo_->setEnabled(!isJson());
    headerCheck_->setEnabled(!isJson());
    reparse();
}

void ImportDialog::reparse() {
    const QString path = pathEdit_->text();
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        infoLabel_->setText(tr("无法读取文件"));
        return;
    }
    const QByteArray bytes = f.readAll();
    f.close();

    if (isJson()) {
        parsed_ = JsonReader::read(bytes);
    } else {
        CsvOptions opt;
        opt.forcedCodec = encodingCombo_->currentData().toString();
        const QString sep = sepCombo_->currentData().toString();
        opt.sep = sep.isEmpty() ? QChar(',') : sep.at(0);
        opt.firstRowHeader = headerCheck_->isChecked();
        parsed_ = CsvReader::read(bytes, opt);
    }

    if (!parsed_.ok) {
        infoLabel_->setText(tr("解析失败: %1").arg(parsed_.error));
        preview_->clear();
        preview_->setRowCount(0);
        preview_->setColumnCount(0);
        mapping_->setRowCount(0);
        importBtn_->setEnabled(false);
        return;
    }

    QString codecInfo = isJson() ? QStringLiteral("JSON")
                                 : tr("编码: %1").arg(parsed_.detectedCodec);
    infoLabel_->setText(tr("%1  共 %2 行,%3 列")
                            .arg(codecInfo).arg(parsed_.rows.size())
                            .arg(parsed_.headers.size()));

    refreshPreview();
    rebuildMapping();
    importBtn_->setEnabled(!parsed_.rows.isEmpty() && !tableCols_.isEmpty());
}

void ImportDialog::refreshPreview() {
    preview_->clear();
    preview_->setColumnCount(parsed_.headers.size());
    preview_->setHorizontalHeaderLabels(parsed_.headers);
    const int n = qMin(kPreviewRows, parsed_.rows.size());
    preview_->setRowCount(n);
    for (int i = 0; i < n; ++i) {
        const QStringList &row = parsed_.rows.at(i);
        for (int j = 0; j < parsed_.headers.size(); ++j)
            preview_->setItem(i, j, cellItem(j < row.size() ? row.at(j) : QString()));
    }
}

void ImportDialog::rebuildMapping() {
    const QMap<QString, int> autoMap =
        ColumnMapping::autoMatch(parsed_.headers, tableCols_);

    mapping_->setRowCount(tableCols_.size());
    for (int i = 0; i < tableCols_.size(); ++i) {
        const QString col = tableCols_.at(i);
        auto *nameItem = new QTableWidgetItem(col);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        mapping_->setItem(i, 0, nameItem);

        auto *combo = new QComboBox;
        combo->addItem(kSkipLabel, -1);
        for (int h = 0; h < parsed_.headers.size(); ++h)
            combo->addItem(parsed_.headers.at(h), h);
        if (autoMap.contains(col))
            combo->setCurrentIndex(autoMap.value(col) + 1);  // +1 跳过“(不导入)”
        mapping_->setCellWidget(i, 1, combo);
    }
}

void ImportDialog::doImport() {
    // 从映射下拉收集 表列 → 文件下标
    QMap<QString, int> mapping;
    for (int i = 0; i < tableCols_.size(); ++i) {
        auto *combo = qobject_cast<QComboBox *>(mapping_->cellWidget(i, 1));
        if (!combo) continue;
        const int fileIdx = combo->currentData().toInt();
        if (fileIdx >= 0) mapping.insert(tableCols_.at(i), fileIdx);
    }
    if (mapping.isEmpty()) {
        QMessageBox::information(this, tr("导入"), tr("请至少映射一列"));
        return;
    }

    QStringList outCols;
    QList<QStringList> outRows;
    ColumnMapping::apply(parsed_.rows, tableCols_, mapping, outCols, outRows);

    QProgressDialog prog(tr("正在导入..."), tr("取消"), 0, outRows.size(), this);
    prog.setWindowModality(Qt::WindowModal);
    prog.setMinimumDuration(0);

    ImportRunner::Executor exec = [this](const QString &sql) {
        QJsonObject req;
        req.insert("funcId", FuncId::EXEC_SQL);
        req.insert("connId", connId_);
        req.insert("sql", sql);
        auto r = client_->call(req);
        return ExecOutcome{ r.ok, r.errorMessage };
    };
    ImportRunner::Progress onProgress = [&prog](int done, int total) {
        prog.setMaximum(total);
        prog.setValue(done);
        QApplication::processEvents();
        return !prog.wasCanceled();
    };

    // 导入期间关闭历史记录,避免 INSERT 刷屏;结束后恢复
    client_->setHistoryEnabled(false);
    ImportResult res = ImportRunner::run(table_, db_, outCols, outRows, dbType_,
                                         parsed_.sourceLines, exec, kBatchSize, onProgress);
    client_->setHistoryEnabled(true);
    prog.close();

    // —— 结果汇总 ——
    QString msg = tr("总计 %1 行,成功 %2 行,失败 %3 行%4")
                      .arg(res.total).arg(res.success).arg(res.failures.size())
                      .arg(res.canceled ? tr("(已取消)") : QString());
    if (res.failures.isEmpty()) {
        QMessageBox::information(this, tr("导入完成"), msg);
    } else {
        QStringList detail;
        for (const FailedRow &fr : res.failures) {
            detail << tr("第 %1 行: %2").arg(fr.line).arg(fr.reason);
            if (detail.size() >= 100) { detail << tr("..."); break; }
        }
        QMessageBox box(QMessageBox::Warning, tr("导入完成(有失败)"), msg, QMessageBox::Ok, this);
        box.setDetailedText(detail.join("\n"));
        box.exec();
    }
}
