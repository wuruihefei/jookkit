#include "export/Exporter.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace {
QString csvField(const QString &v, QChar sep) {
    bool needQuote = v.contains(sep) || v.contains('"') ||
                     v.contains('\n') || v.contains('\r');
    if (!needQuote) return v;
    QString s = v;
    s.replace("\"", "\"\"");
    return "\"" + s + "\"";
}

QString quoteIdent(const QString &id, const QString &dbType) {
    if (dbType == "sqlite")
        return "\"" + QString(id).replace("\"", "\"\"") + "\"";
    return "`" + QString(id).replace("`", "``") + "`";   // mysql/默认
}

QString quoteVal(const QString &v) {
    if (v.isNull()) return QStringLiteral("NULL");   // null QString = SQL NULL
    return "'" + QString(v).replace("'", "''") + "'"; // 含空串"":输出 ''
}
}

QByteArray Exporter::toCsv(const QStringList &headers, const QList<QStringList> &rows,
                           QChar sep, bool withHeader, bool bom) {
    QString out;
    if (withHeader) {
        QStringList hs;
        for (const QString &h : headers) hs << csvField(h, sep);
        out += hs.join(sep) + "\r\n";
    }
    for (const QStringList &row : rows) {
        QStringList cs;
        for (const QString &c : row) cs << csvField(c, sep);
        out += cs.join(sep) + "\r\n";
    }
    QByteArray bytes = out.toUtf8();
    if (bom) bytes.prepend("\xEF\xBB\xBF", 3);
    return bytes;
}

QByteArray Exporter::toJson(const QStringList &headers, const QList<QStringList> &rows) {
    QJsonArray arr;
    for (const QStringList &row : rows) {
        QJsonObject o;
        for (int j = 0; j < headers.size(); ++j)
            o.insert(headers.at(j), j < row.size() ? row.at(j) : QString());
        arr.append(o);
    }
    return QJsonDocument(arr).toJson(QJsonDocument::Indented);
}

QByteArray Exporter::toInsertSql(const QString &table, const QStringList &headers,
                                 const QList<QStringList> &rows,
                                 const QString &dbType, bool batch, const QString &db) {
    QStringList cols;
    for (const QString &h : headers) cols << quoteIdent(h, dbType);
    const QString tbl = db.isEmpty()
        ? quoteIdent(table, dbType)
        : quoteIdent(db, dbType) + "." + quoteIdent(table, dbType);
    const QString colClause = "(" + cols.join(", ") + ")";

    auto valuesOf = [&headers](const QStringList &row) {
        QStringList vs;
        for (int j = 0; j < headers.size(); ++j)
            vs << quoteVal(j < row.size() ? row.at(j) : QString());
        return "(" + vs.join(", ") + ")";
    };

    QString out;
    if (batch) {
        QStringList tuples;
        for (const QStringList &row : rows) tuples << valuesOf(row);
        if (!tuples.isEmpty())
            out = "INSERT INTO " + tbl + " " + colClause +
                  " VALUES " + tuples.join(", ") + ";\n";
    } else {
        for (const QStringList &row : rows)
            out += "INSERT INTO " + tbl + " " + colClause +
                   " VALUES " + valuesOf(row) + ";\n";
    }
    return out.toUtf8();
}
