#include "ui/TableStructureForm.h"
#include "backend/BackendClient.h"
#include "backend/FuncId.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>

TableStructureForm::TableStructureForm(BackendClient *client, const QString &connId,
                                       const QString &db, const QString &table,
                                       QWidget *parent)
    : QWidget(parent), client_(client), connId_(connId), db_(db), table_(table) {
    grid_ = new QTableWidget;
    grid_->setColumnCount(4);
    grid_->setHorizontalHeaderLabels({tr("列名"), tr("类型"), tr("可空"), tr("主键")});
    grid_->horizontalHeader()->setStretchLastSection(true);
    grid_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(grid_);
    load();
}

void TableStructureForm::load() {
    if (!client_) return;
    QJsonObject req;
    req.insert("funcId", FuncId::DESCRIBE_TABLE);
    req.insert("connId", connId_);
    req.insert("db", db_);
    req.insert("table", table_);
    auto r = client_->call(req);
    if (!r.ok) return;

    QSet<QString> pks;
    for (const auto &p : r.data.value("primaryKeys").toArray())
        pks.insert(p.toString());

    QJsonArray cols = r.data.value("columns").toArray();
    grid_->setRowCount(cols.size());
    for (int i = 0; i < cols.size(); ++i) {
        QJsonObject c = cols.at(i).toObject();
        QString name = c.value("name").toString();
        grid_->setItem(i, 0, new QTableWidgetItem(name));
        grid_->setItem(i, 1, new QTableWidgetItem(c.value("type").toString()));
        grid_->setItem(i, 2, new QTableWidgetItem(
            c.value("nullable").toBool() ? tr("是") : tr("否")));
        grid_->setItem(i, 3, new QTableWidgetItem(pks.contains(name) ? "PK" : ""));
    }
}
