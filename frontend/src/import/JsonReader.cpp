#include "import/JsonReader.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>
#include <QSet>

namespace {

// 单个 JSON 值 → 单元格文本。null/缺键 → QString()(NULL)。
QString cellFromValue(const QJsonValue &v) {
    if (v.isUndefined() || v.isNull()) return QString();          // NULL
    if (v.isString()) {
        QString s = v.toString();
        return s.isNull() ? QString("") : s;                      // "" 保持非 null
    }
    if (v.isBool()) return v.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (v.isDouble()) {
        double d = v.toDouble();
        qint64 ll = (qint64)d;
        if ((double)ll == d) return QString::number(ll);          // 整数不带 .0
        return QString::number(d, 'g', 15);
    }
    if (v.isArray())  return QString::fromUtf8(QJsonDocument(v.toArray()).toJson(QJsonDocument::Compact));
    if (v.isObject()) return QString::fromUtf8(QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact));
    return QString();
}

} // namespace

namespace JsonReader {

ParseResult read(const QByteArray &bytes) {
    ParseResult res;
    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &perr);
    if (perr.error != QJsonParseError::NoError) {
        res.ok = false;
        res.error = QStringLiteral("JSON 解析错误:") + perr.errorString();
        return res;
    }
    if (!doc.isArray()) {
        res.ok = false;
        res.error = QStringLiteral("顶层必须是对象数组 [ {...}, ... ]");
        return res;
    }

    const QJsonArray arr = doc.array();
    QList<QJsonObject> objs;
    QSet<QString> seen;
    for (const QJsonValue &v : arr) {
        if (!v.isObject()) {
            res.ok = false;
            res.error = QStringLiteral("数组元素必须是对象");
            return res;
        }
        QJsonObject o = v.toObject();
        objs << o;
        for (const QString &key : o.keys())
            if (!seen.contains(key)) { seen.insert(key); res.headers << key; }
    }

    int line = 1;
    for (const QJsonObject &o : objs) {
        QStringList row;
        for (const QString &h : res.headers)
            row << cellFromValue(o.value(h));            // 缺键 → Undefined → NULL
        res.rows << row;
        res.sourceLines << line++;                       // JSON 无物理行:用元素序号
    }
    return res;
}

} // namespace JsonReader
