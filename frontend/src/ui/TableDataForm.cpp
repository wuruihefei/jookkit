#include "ui/TableDataForm.h"
#include "ui/Icons.h"
#include "backend/BackendClient.h"
#include "backend/FuncId.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QToolBar>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>

TableDataForm::TableDataForm(BackendClient *client, const QString &connId,
                             const QString &db, const QString &table, QWidget *parent)
    : QWidget(parent), client_(client), connId_(connId), db_(db), table_(table) {

    pageSize_ = QSettings().value("data/pageSize", 200).toInt();
    if (pageSize_ <= 0) pageSize_ = 200;

    auto *toolbar = new QToolBar;
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(Icons::refresh(), tr("刷新"), this, &TableDataForm::reload);
    toolbar->addAction(Icons::add(), tr("新增行"), this, &TableDataForm::addRow);
    toolbar->addAction(Icons::save(), tr("保存新行"), this, &TableDataForm::saveNewRows);
    toolbar->addAction(Icons::remove(), tr("删除行"), this, &TableDataForm::deleteSelectedRow);
    toolbar->addSeparator();
    toolbar->addAction(tr("◀ 上一页"), this, &TableDataForm::prevPage);
    toolbar->addAction(tr("下一页 ▶"), this, &TableDataForm::nextPage);
    pageLabel_ = new QLabel(tr("第 1 页"));
    toolbar->addWidget(pageLabel_);
    toolbar->addSeparator();
    toolbar->addWidget(new QLabel(tr(" 每页 ")));
    auto *sizeCombo = new QComboBox;
    sizeCombo->addItems({"100", "200", "500", "1000"});
    int defIdx = sizeCombo->findText(QString::number(pageSize_));
    sizeCombo->setCurrentIndex(defIdx >= 0 ? defIdx : 1);
    connect(sizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TableDataForm::changePageSize);
    toolbar->addWidget(sizeCombo);

    grid_ = new QTableWidget;
    grid_->horizontalHeader()->setStretchLastSection(true);
    status_ = new QLabel(tr("就绪"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(toolbar);
    layout->addWidget(grid_, 1);
    layout->addWidget(status_);

    connect(grid_, &QTableWidget::itemChanged, this, &TableDataForm::onItemChanged);
    reload();
}

void TableDataForm::changePageSize(int idx) {
    const int sizes[] = {100, 200, 500, 1000};
    if (idx >= 0 && idx < 4) {
        pageSize_ = sizes[idx];
        page_ = 0;
        reload();
    }
}

void TableDataForm::prevPage() {
    if (page_ > 0) { --page_; reload(); }
}

void TableDataForm::nextPage() {
    ++page_;
    reload();
}

void TableDataForm::updatePageLabel(int rowsThisPage) {
    pageLabel_->setText(tr(" 第 %1 页 (本页 %2 行) ").arg(page_ + 1).arg(rowsThisPage));
}

void TableDataForm::reload() {
    if (!client_) return;
    loading_ = true;
    newRows_.clear();
    rowOriginals_.clear();

    primaryKeys_.clear();
    {
        QJsonObject req;
        req.insert("funcId", FuncId::DESCRIBE_TABLE);
        req.insert("connId", connId_);
        req.insert("db", db_);
        req.insert("table", table_);
        auto r = client_->call(req);
        if (r.ok)
            for (const auto &p : r.data.value("primaryKeys").toArray())
                primaryKeys_ << p.toString();
    }

    QJsonObject req;
    req.insert("funcId", FuncId::EXEC_SQL);
    req.insert("connId", connId_);
    req.insert("sql", QString("select * from %1 limit %2 offset %3")
               .arg(table_).arg(pageSize_).arg(page_ * pageSize_));
    auto r = client_->call(req);
    if (!r.ok) {
        status_->setText(tr("加载失败: %1").arg(r.errorMessage));
        loading_ = false;
        return;
    }

    QJsonArray cols = r.data.value("columns").toArray();
    QJsonArray rows = r.data.value("rows").toArray();

    // 若翻过头(空页)且非首页,回退一页
    if (rows.isEmpty() && page_ > 0) {
        --page_;
        loading_ = false;
        reload();
        return;
    }

    columns_.clear();
    for (const auto &c : cols) columns_ << c.toObject().value("name").toString();

    grid_->clear();
    grid_->setColumnCount(columns_.size());
    grid_->setHorizontalHeaderLabels(columns_);
    grid_->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        QJsonArray row = rows.at(i).toArray();
        QMap<QString, QString> orig;
        for (int j = 0; j < columns_.size() && j < row.size(); ++j) {
            const QJsonValue v = row.at(j);
            QString s = v.isNull() ? QString() : v.toVariant().toString();
            grid_->setItem(i, j, new QTableWidgetItem(s));
            orig.insert(columns_.at(j), s);
        }
        rowOriginals_.append(orig);
    }

    updatePageLabel(rows.size());
    QString pkNote = primaryKeys_.isEmpty()
        ? tr("(无主键,只读)") : tr("(主键: %1)").arg(primaryKeys_.join(","));
    status_->setText(tr("已加载 %1 行 %2").arg(rows.size()).arg(pkNote));
    grid_->setEditTriggers(primaryKeys_.isEmpty()
        ? QAbstractItemView::NoEditTriggers
        : (QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed));

    loading_ = false;
}

QMap<QString, QString> TableDataForm::pkOf(int row) const {
    QMap<QString, QString> pk;
    if (row < 0 || row >= rowOriginals_.size()) return pk;
    for (const QString &k : primaryKeys_)
        pk.insert(k, rowOriginals_.at(row).value(k));
    return pk;
}

static QJsonObject toJson(const QMap<QString, QString> &m) {
    QJsonObject o;
    for (auto it = m.begin(); it != m.end(); ++it) o.insert(it.key(), it.value());
    return o;
}

void TableDataForm::onItemChanged(QTableWidgetItem *item) {
    if (loading_) return;
    int row = item->row();
    int col = item->column();
    if (newRows_.contains(row)) return;
    if (primaryKeys_.isEmpty()) return;

    const QString colName = columns_.at(col);
    QJsonObject values; values.insert(colName, item->text());

    QJsonObject req;
    req.insert("funcId", FuncId::UPDATE_ROW);
    req.insert("connId", connId_);
    req.insert("table", table_);
    req.insert("values", values);
    req.insert("pk", toJson(pkOf(row)));
    auto r = client_->call(req);
    if (!r.ok) { status_->setText(tr("更新失败: %1").arg(r.errorMessage)); return; }
    rowOriginals_[row].insert(colName, item->text());
    status_->setText(tr("已更新 %1 行").arg(r.data.value("affected").toInt()));
}

void TableDataForm::addRow() {
    if (columns_.isEmpty()) { status_->setText(tr("请先加载数据")); return; }
    loading_ = true;
    int row = grid_->rowCount();
    grid_->insertRow(row);
    for (int j = 0; j < columns_.size(); ++j)
        grid_->setItem(row, j, new QTableWidgetItem());
    rowOriginals_.append(QMap<QString, QString>());
    newRows_.insert(row);
    loading_ = false;
    status_->setText(tr("已添加空行,填写后点'保存新行'"));
}

void TableDataForm::saveNewRows() {
    if (newRows_.isEmpty()) { status_->setText(tr("没有待保存的新行")); return; }
    int inserted = 0;
    const QList<int> rows = newRows_.values();
    for (int row : rows) {
        QJsonObject values;
        for (int j = 0; j < columns_.size(); ++j) {
            auto *it = grid_->item(row, j);
            QString t = it ? it->text() : QString();
            if (!t.isEmpty()) values.insert(columns_.at(j), t);
        }
        if (values.isEmpty()) continue;
        QJsonObject req;
        req.insert("funcId", FuncId::INSERT_ROW);
        req.insert("connId", connId_);
        req.insert("table", table_);
        req.insert("values", values);
        auto r = client_->call(req);
        if (!r.ok) { status_->setText(tr("插入失败: %1").arg(r.errorMessage)); return; }
        ++inserted;
    }
    status_->setText(tr("已插入 %1 行,刷新中").arg(inserted));
    reload();
}

void TableDataForm::deleteSelectedRow() {
    int row = grid_->currentRow();
    if (row < 0) { status_->setText(tr("请先选中一行")); return; }
    if (newRows_.contains(row)) {
        loading_ = true;
        grid_->removeRow(row);
        rowOriginals_.removeAt(row);
        newRows_.remove(row);
        loading_ = false;
        return;
    }
    if (primaryKeys_.isEmpty()) { status_->setText(tr("无主键,无法删除")); return; }

    QJsonObject req;
    req.insert("funcId", FuncId::DELETE_ROW);
    req.insert("connId", connId_);
    req.insert("table", table_);
    req.insert("pk", toJson(pkOf(row)));
    auto r = client_->call(req);
    if (!r.ok) { status_->setText(tr("删除失败: %1").arg(r.errorMessage)); return; }
    status_->setText(tr("已删除 %1 行,刷新中").arg(r.data.value("affected").toInt()));
    reload();
}
