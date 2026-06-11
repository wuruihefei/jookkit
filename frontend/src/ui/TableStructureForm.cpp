#include "ui/TableStructureForm.h"
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
#include <QRegularExpression>
#include <QSet>

namespace {
const int kColName = 0, kColType = 1, kColNullable = 2, kColPk = 3;
// 列名单元格上挂载的原始定义(load 时快照,保存时据此判定变更)
const int kOrigNameRole     = Qt::UserRole;
const int kOrigTypeRole     = Qt::UserRole + 1;
const int kOrigNullableRole = Qt::UserRole + 2;
const int kPkRole           = Qt::UserRole + 3;
const int kOrigDefaultRole  = Qt::UserRole + 4;
const int kOrigExtraRole    = Qt::UserRole + 5;
const int kOrigCommentRole  = Qt::UserRole + 6;
}

TableStructureForm::TableStructureForm(BackendClient *client, const QString &connId,
                                       const QString &db, const QString &table,
                                       const QString &dbType, QWidget *parent)
    : QWidget(parent), client_(client), connId_(connId), db_(db), table_(table),
      dbType_(dbType.toLower()) {
    editable_ = isMysql() || isSqlite();

    auto *toolbar = new QToolBar;
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(Icons::refresh(), tr("刷新"), this, &TableStructureForm::load);
    if (editable_) {
        toolbar->addSeparator();
        toolbar->addAction(Icons::add(), tr("新增字段"), this, &TableStructureForm::addField);
        toolbar->addAction(Icons::remove(), tr("删除字段"), this, &TableStructureForm::removeField);
        toolbar->addAction(Icons::save(), tr("保存修改"), this, &TableStructureForm::saveChanges);
    }

    grid_ = new QTableWidget;
    grid_->setColumnCount(4);
    grid_->setHorizontalHeaderLabels({tr("列名"), tr("类型"), tr("可空"), tr("主键")});
    grid_->horizontalHeader()->setStretchLastSection(true);
    grid_->setColumnWidth(kColType, 180);
    grid_->setSelectionBehavior(QAbstractItemView::SelectRows);
    if (!editable_)
        grid_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    status_ = new QLabel(tr("就绪"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(toolbar);
    layout->addWidget(grid_, 1);
    layout->addWidget(status_);
    load();
}

QStringList TableStructureForm::typeOptions() const {
    if (isSqlite())
        return {"INTEGER", "TEXT", "REAL", "NUMERIC", "BLOB"};
    return {"INT", "BIGINT", "SMALLINT", "TINYINT", "DECIMAL(10,2)", "FLOAT", "DOUBLE",
            "VARCHAR(255)", "CHAR(36)", "TEXT", "MEDIUMTEXT", "LONGTEXT",
            "DATE", "DATETIME", "TIMESTAMP", "TIME", "YEAR",
            "JSON", "BLOB", "LONGBLOB", "BOOLEAN"};
}

QStringList TableStructureForm::allowedBaseTypes() const {
    if (isSqlite())
        return {"INTEGER", "INT", "TEXT", "REAL", "NUMERIC", "BLOB"};
    return {"INT", "INTEGER", "BIGINT", "SMALLINT", "TINYINT", "MEDIUMINT",
            "DECIMAL", "NUMERIC", "FLOAT", "DOUBLE", "BIT",
            "VARCHAR", "CHAR", "BINARY", "VARBINARY",
            "TEXT", "TINYTEXT", "MEDIUMTEXT", "LONGTEXT",
            "DATE", "DATETIME", "TIMESTAMP", "TIME", "YEAR",
            "JSON", "BLOB", "TINYBLOB", "MEDIUMBLOB", "LONGBLOB",
            "BOOLEAN", "BOOL", "ENUM", "SET"};
}

QString TableStructureForm::quoteIdent(const QString &id) const {
    if (isMysql()) return "`" + QString(id).replace("`", "``") + "`";
    return "\"" + QString(id).replace("\"", "\"\"") + "\"";
}

QString TableStructureForm::quoteTable() const {
    if (isMysql() && !db_.isEmpty())
        return quoteIdent(db_) + "." + quoteIdent(table_);
    return quoteIdent(table_);
}

QString TableStructureForm::quoteLiteral(QString s) {
    return "'" + s.replace("'", "''").replace("\\", "\\\\") + "'";
}

// 经 information_schema 取 MySQL 完整列定义(COLUMN_TYPE 含长度/精度;
// JDBC DESCRIBE 的 TYPE_NAME 不含,直接用于 CHANGE 会损毁精度/报语法错)。
bool TableStructureForm::fetchMysqlColumnMeta(QHash<QString, ColMeta> &out) {
    QJsonObject req;
    req.insert("funcId", FuncId::EXEC_SQL);
    req.insert("connId", connId_);
    req.insert("sql", QString(
        "SELECT COLUMN_NAME, COLUMN_TYPE, IS_NULLABLE, COLUMN_DEFAULT, EXTRA, COLUMN_COMMENT "
        "FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=%1 AND TABLE_NAME=%2")
        .arg(quoteLiteral(db_), quoteLiteral(table_)));
    auto r = client_->call(req);
    if (!r.ok) return false;
    for (const auto &v : r.data.value("rows").toArray()) {
        QJsonArray row = v.toArray();
        if (row.size() < 6) continue;
        ColMeta m;
        m.columnType = row.at(1).toVariant().toString();
        m.nullable = row.at(2).toVariant().toString().compare("YES", Qt::CaseInsensitive) == 0;
        m.defaultValue = row.at(3).isNull() ? QVariant() : row.at(3).toVariant();
        m.extra = row.at(4).toVariant().toString();
        m.comment = row.at(5).toVariant().toString();
        out.insert(row.at(0).toVariant().toString(), m);
    }
    return true;
}

void TableStructureForm::setRow(int row, const RowData &rd) {
    auto *nameItem = new QTableWidgetItem(rd.name);
    nameItem->setData(kOrigNameRole, rd.origName);
    nameItem->setData(kOrigTypeRole, rd.origType);
    nameItem->setData(kOrigNullableRole, rd.origNullable);
    nameItem->setData(kPkRole, rd.pk);
    nameItem->setData(kOrigDefaultRole, rd.origDefault);
    nameItem->setData(kOrigExtraRole, rd.origExtra);
    nameItem->setData(kOrigCommentRole, rd.origComment);
    grid_->setItem(row, kColName, nameItem);

    if (editable_) {
        auto *combo = new QComboBox;
        combo->setEditable(true);
        combo->addItems(typeOptions());
        combo->setCurrentText(rd.type);
        grid_->setCellWidget(row, kColType, combo);
        grid_->setItem(row, kColType, new QTableWidgetItem);  // 占位,便于行选中
    } else {
        grid_->setItem(row, kColType, new QTableWidgetItem(rd.type));
    }

    auto *nullItem = new QTableWidgetItem;
    Qt::ItemFlags nf = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (editable_) nf |= Qt::ItemIsUserCheckable;
    nullItem->setFlags(nf);
    nullItem->setCheckState(rd.nullable ? Qt::Checked : Qt::Unchecked);
    grid_->setItem(row, kColNullable, nullItem);

    auto *pkItem = new QTableWidgetItem(rd.pk ? "PK" : "");
    pkItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    grid_->setItem(row, kColPk, pkItem);
}

void TableStructureForm::load() {
    if (!client_) return;
    dropped_.clear();
    QJsonObject req;
    req.insert("funcId", FuncId::DESCRIBE_TABLE);
    req.insert("connId", connId_);
    req.insert("db", db_);
    req.insert("table", table_);
    auto r = client_->call(req);
    if (!r.ok) {
        status_->setText(tr("加载失败: %1").arg(r.errorMessage));
        return;
    }

    QHash<QString, ColMeta> fullMeta;
    fullMetaOk_ = isMysql() ? fetchMysqlColumnMeta(fullMeta) : false;

    QSet<QString> pks;
    for (const auto &p : r.data.value("primaryKeys").toArray())
        pks.insert(p.toString());

    QJsonArray cols = r.data.value("columns").toArray();
    grid_->setRowCount(cols.size());
    for (int i = 0; i < cols.size(); ++i) {
        QJsonObject c = cols.at(i).toObject();
        RowData rd;
        rd.name = rd.origName = c.value("name").toString();
        rd.type = rd.origType = c.value("type").toString();
        rd.nullable = rd.origNullable = c.value("nullable").toBool();
        rd.pk = pks.contains(rd.name);
        auto it = fullMeta.constFind(rd.name);
        if (it != fullMeta.constEnd()) {
            rd.type = rd.origType = it->columnType;   // 用完整类型(含长度/精度)展示
            rd.nullable = rd.origNullable = it->nullable;
            rd.origDefault = it->defaultValue;
            rd.origExtra = it->extra;
            rd.origComment = it->comment;
        }
        setRow(i, rd);
    }
    if (isMysql() && !fullMetaOk_)
        status_->setText(tr("共 %1 个字段 (未取到完整列定义,修改类型/可空已禁用)").arg(cols.size()));
    else
        status_->setText(tr("共 %1 个字段").arg(cols.size()));
}

void TableStructureForm::addField() {
    int row = grid_->rowCount();
    grid_->setRowCount(row + 1);
    RowData rd;
    rd.type = typeOptions().first();
    setRow(row, rd);
    grid_->setCurrentCell(row, kColName);
    grid_->editItem(grid_->item(row, kColName));
}

void TableStructureForm::removeField() {
    int row = grid_->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("删除字段"), tr("请先选中一个字段"));
        return;
    }
    QString origName = grid_->item(row, kColName)->data(kOrigNameRole).toString();
    if (!origName.isEmpty()) {
        if (QMessageBox::question(this, tr("删除字段"),
                tr("将在保存时删除列 \"%1\",确认?").arg(origName)) != QMessageBox::Yes)
            return;
        dropped_ << origName;
    }
    grid_->removeRow(row);
}

bool TableStructureForm::collectRows(QList<RowData> &rows, QString &error) const {
    static const QRegularExpression identRe("^[A-Za-z_][A-Za-z0-9_$]*$");
    // 基础类型 + 可选参数,如 VARCHAR(255)、DECIMAL(10,2)、ENUM('a','b')
    static const QRegularExpression typeRe(
        "^([A-Za-z]+)\\s*(\\((\\s*\\d+\\s*(,\\s*\\d+\\s*)?|\\s*'[^']*'(\\s*,\\s*'[^']*')*\\s*)\\))?"
        "(\\s+(UNSIGNED|ZEROFILL))*$", QRegularExpression::CaseInsensitiveOption);

    QSet<QString> seen;
    const QStringList allowed = allowedBaseTypes();
    for (int i = 0; i < grid_->rowCount(); ++i) {
        QTableWidgetItem *nameItem = grid_->item(i, kColName);
        RowData rd;
        rd.name = nameItem->text().trimmed();
        rd.origName = nameItem->data(kOrigNameRole).toString();
        rd.origType = nameItem->data(kOrigTypeRole).toString();
        rd.origNullable = nameItem->data(kOrigNullableRole).toBool();
        rd.pk = nameItem->data(kPkRole).toBool();
        rd.origDefault = nameItem->data(kOrigDefaultRole);
        rd.origExtra = nameItem->data(kOrigExtraRole).toString();
        rd.origComment = nameItem->data(kOrigCommentRole).toString();
        auto *combo = qobject_cast<QComboBox *>(grid_->cellWidget(i, kColType));
        rd.type = combo ? combo->currentText().trimmed()
                        : grid_->item(i, kColType)->text().trimmed();
        rd.nullable = grid_->item(i, kColNullable)->checkState() == Qt::Checked;

        const bool isNew = rd.origName.isEmpty();
        const bool renamed = !isNew && rd.name != rd.origName;
        const bool typeChanged = !isNew &&
            rd.type.compare(rd.origType, Qt::CaseInsensitive) != 0;
        const bool nullChanged = !isNew && rd.nullable != rd.origNullable;

        // 列名:总是查空与重名;格式仅在新增/改名时查(已有列名以库内为准)
        if (rd.name.isEmpty()) {
            error = tr("第 %1 行:列名不能为空").arg(i + 1);
            return false;
        }
        if ((isNew || renamed) && !identRe.match(rd.name).hasMatch()) {
            error = tr("第 %1 行:列名 \"%2\" 不合法(须以字母/下划线开头,仅含字母数字下划线)")
                        .arg(i + 1).arg(rd.name);
            return false;
        }
        QString lower = rd.name.toLower();
        if (seen.contains(lower)) {
            error = tr("列名 \"%1\" 重复").arg(rd.name);
            return false;
        }
        seen.insert(lower);

        // 类型:仅校验新增或被改动的行,未动的列不受白名单限制
        if (isNew || typeChanged) {
            if (rd.type.isEmpty()) {
                error = tr("第 %1 行:类型不能为空").arg(i + 1);
                return false;
            }
            auto m = typeRe.match(rd.type);
            if (!m.hasMatch() || !allowed.contains(m.captured(1).toUpper())) {
                error = tr("第 %1 行:类型 \"%2\" 与当前数据源(%3)不匹配")
                            .arg(i + 1).arg(rd.type, dbType_);
                return false;
            }
        }

        // 主键列只允许改名(类型/可空涉及 AUTO_INCREMENT/索引,暂不支持)
        if (rd.pk && (typeChanged || nullChanged)) {
            error = tr("暂不支持修改主键列 \"%1\" 的类型/可空性").arg(rd.origName);
            return false;
        }

        // MySQL 未取到完整列定义时,CHANGE 重建会丢长度/精度(如 DECIMAL→(10,0)),禁止
        if (isMysql() && !fullMetaOk_ && (typeChanged || nullChanged)) {
            error = tr("未取到完整列定义,暂不能修改类型/可空(改名/增删字段不受影响),请刷新重试");
            return false;
        }
        rows << rd;
    }
    return true;
}

QStringList TableStructureForm::buildAlterSql(const QList<RowData> &rows, QString &error) const {
    QStringList sqls;
    const QString qt = quoteTable();

    for (const QString &col : dropped_)
        sqls << QString("ALTER TABLE %1 DROP COLUMN %2").arg(qt, quoteIdent(col));

    for (const RowData &rd : rows) {
        if (rd.origName.isEmpty()) {  // 新增列
            if (isSqlite() && !rd.nullable) {
                error = tr("SQLite 新增非空字段需要默认值,暂不支持,请将 \"%1\" 设为可空")
                            .arg(rd.name);
                return {};
            }
            QString def = QString("%1 %2").arg(quoteIdent(rd.name), rd.type);
            if (!rd.nullable) def += " NOT NULL";
            sqls << QString("ALTER TABLE %1 ADD COLUMN %2").arg(qt, def);
            continue;
        }
        bool renamed = (rd.name != rd.origName);
        bool typeChanged = (rd.type.compare(rd.origType, Qt::CaseInsensitive) != 0);
        bool nullChanged = (rd.nullable != rd.origNullable);
        if (!renamed && !typeChanged && !nullChanged) continue;

        if (isSqlite()) {
            if (typeChanged || nullChanged) {
                error = tr("SQLite 不支持修改已有列的类型/可空性(列 \"%1\")").arg(rd.origName);
                return {};
            }
            sqls << QString("ALTER TABLE %1 RENAME COLUMN %2 TO %3")
                        .arg(qt, quoteIdent(rd.origName), quoteIdent(rd.name));
            continue;
        }

        // MySQL:纯改名走 RENAME COLUMN(8.0+,完整保留列定义);
        // 改类型/可空才用 CHANGE 重建定义,并尽量保留默认值/注释
        if (renamed && !typeChanged && !nullChanged) {
            sqls << QString("ALTER TABLE %1 RENAME COLUMN %2 TO %3")
                        .arg(qt, quoteIdent(rd.origName), quoteIdent(rd.name));
            continue;
        }
        QString def = QString("%1 %2").arg(rd.type, rd.nullable ? "NULL" : "NOT NULL");
        if (fullMetaOk_ && rd.origDefault.isValid()) {
            QString dv = rd.origDefault.toString();
            bool generated = rd.origExtra.contains("DEFAULT_GENERATED", Qt::CaseInsensitive)
                             || dv.startsWith("CURRENT_TIMESTAMP", Qt::CaseInsensitive);
            def += generated ? QString(" DEFAULT %1").arg(dv)
                             : QString(" DEFAULT %1").arg(quoteLiteral(dv));
        }
        // ON UPDATE/AUTO_INCREMENT 独立于 DEFAULT 存在(如 TIMESTAMP NULL ON UPDATE、UNIQUE 自增列)
        if (fullMetaOk_ && rd.origExtra.contains("on update CURRENT_TIMESTAMP", Qt::CaseInsensitive))
            def += " ON UPDATE CURRENT_TIMESTAMP";
        if (fullMetaOk_ && rd.origExtra.contains("auto_increment", Qt::CaseInsensitive))
            def += " AUTO_INCREMENT";
        if (fullMetaOk_ && !rd.origComment.isEmpty())
            def += QString(" COMMENT %1").arg(quoteLiteral(rd.origComment));
        sqls << QString("ALTER TABLE %1 CHANGE %2 %3 %4")
                    .arg(qt, quoteIdent(rd.origName), quoteIdent(rd.name), def);
    }
    return sqls;
}

void TableStructureForm::saveChanges() {
    if (!client_ || !editable_) return;
    QList<RowData> rows;
    QString error;
    if (!collectRows(rows, error)) {
        QMessageBox::warning(this, tr("校验失败"), error);
        return;
    }
    QStringList sqls = buildAlterSql(rows, error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("校验失败"), error);
        return;
    }
    if (sqls.isEmpty()) {
        status_->setText(tr("没有需要保存的修改"));
        return;
    }
    if (QMessageBox::question(this, tr("保存修改"),
            tr("将执行以下 %1 条语句(DDL 不可回滚,失败即中止):\n\n%2\n\n确认?")
                .arg(sqls.size()).arg(sqls.join(";\n"))) != QMessageBox::Yes)
        return;

    for (const QString &sql : sqls) {
        QJsonObject req;
        req.insert("funcId", FuncId::EXEC_SQL);
        req.insert("connId", connId_);
        req.insert("sql", sql);
        auto r = client_->call(req);
        if (!r.ok) {
            QMessageBox::critical(this, tr("执行失败"),
                tr("语句执行失败,后续语句已中止:\n%1\n\n%2").arg(sql, r.errorMessage));
            load();
            return;
        }
    }
    status_->setText(tr("已保存 %1 处修改").arg(sqls.size()));
    load();
}
