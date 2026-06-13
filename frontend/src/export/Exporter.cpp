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
                                 const QString &dbType, bool batch) {
    Q_UNUSED(table)
    Q_UNUSED(headers)
    Q_UNUSED(rows)
    Q_UNUSED(dbType)
    Q_UNUSED(batch)
    return QByteArray(); // 在 Task 1.3 中实现
}
