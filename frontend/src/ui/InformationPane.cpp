#include "ui/InformationPane.h"
#include "backend/BackendClient.h"
#include "backend/FuncId.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>

InformationPane::InformationPane(BackendClient *client, QWidget *parent)
    : QTabWidget(parent), client_(client) {
    setDocumentMode(true);

    general_ = new QTableWidget;
    general_->setColumnCount(4);
    general_->setHorizontalHeaderLabels({tr("列名"), tr("类型"), tr("可空"), tr("主键")});
    general_->horizontalHeader()->setStretchLastSection(true);
    general_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    general_->verticalHeader()->setVisible(false);

    indexes_ = new QTableWidget;
    indexes_->setColumnCount(3);
    indexes_->setHorizontalHeaderLabels({tr("索引"), tr("列"), tr("唯一")});
    indexes_->horizontalHeader()->setStretchLastSection(true);
    indexes_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    indexes_->verticalHeader()->setVisible(false);

    ddl_ = new QPlainTextEdit;
    ddl_->setReadOnly(true);
    QFont f("Monospace"); f.setStyleHint(QFont::TypeWriter);
    ddl_->setFont(f);

    addTab(general_, tr("常规"));
    addTab(indexes_, tr("索引"));
    addTab(ddl_, tr("DDL"));
}

void InformationPane::clearInfo() {
    general_->setRowCount(0);
    indexes_->setRowCount(0);
    ddl_->clear();
}

void InformationPane::showTable(const QString &connId, const QString &db, const QString &table) {
    if (!client_) return;

    // 常规:列信息
    QJsonObject dreq;
    dreq.insert("funcId", FuncId::DESCRIBE_TABLE);
    dreq.insert("connId", connId);
    dreq.insert("db", db);
    dreq.insert("table", table);
    auto dr = client_->call(dreq);
    general_->setRowCount(0);
    if (dr.ok) {
        QSet<QString> pks;
        for (const auto &p : dr.data.value("primaryKeys").toArray()) pks.insert(p.toString());
        QJsonArray cols = dr.data.value("columns").toArray();
        general_->setRowCount(cols.size());
        for (int i = 0; i < cols.size(); ++i) {
            QJsonObject c = cols.at(i).toObject();
            QString name = c.value("name").toString();
            general_->setItem(i, 0, new QTableWidgetItem(name));
            general_->setItem(i, 1, new QTableWidgetItem(c.value("type").toString()));
            general_->setItem(i, 2, new QTableWidgetItem(c.value("nullable").toBool() ? tr("是") : tr("否")));
            general_->setItem(i, 3, new QTableWidgetItem(pks.contains(name) ? "PK" : ""));
        }
    }

    // DDL
    QJsonObject ddlReq;
    ddlReq.insert("funcId", FuncId::GET_DDL);
    ddlReq.insert("connId", connId);
    ddlReq.insert("db", db);
    ddlReq.insert("table", table);
    auto er = client_->call(ddlReq);
    ddl_->setPlainText(er.ok ? er.data.value("ddl").toString() : er.errorMessage);

    // 索引(GET_SCHEMA)
    QJsonObject sreq;
    sreq.insert("funcId", FuncId::GET_SCHEMA);
    sreq.insert("connId", connId);
    sreq.insert("db", db);
    sreq.insert("table", table);
    auto sr = client_->call(sreq);
    indexes_->setRowCount(0);
    if (sr.ok) {
        QJsonArray idx = sr.data.value("indexes").toArray();
        indexes_->setRowCount(idx.size());
        for (int i = 0; i < idx.size(); ++i) {
            QJsonObject o = idx.at(i).toObject();
            QStringList colNames;
            for (const auto &cn : o.value("columns").toArray()) colNames << cn.toString();
            indexes_->setItem(i, 0, new QTableWidgetItem(o.value("name").toString()));
            indexes_->setItem(i, 1, new QTableWidgetItem(colNames.join(", ")));
            indexes_->setItem(i, 2, new QTableWidgetItem(o.value("unique").toBool() ? tr("是") : tr("否")));
        }
    }
}
