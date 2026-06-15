#include "store/HistoryStore.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

namespace {
QString filePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (dir.isEmpty()) dir = QDir::homePath() + "/.jookkit";
    QDir().mkpath(dir);
    return dir + "/history.jsonl";
}

QJsonObject toObj(const HistoryEntry &e) {
    QJsonObject o;
    o.insert("sql", e.sql);
    o.insert("connId", e.connId);
    o.insert("db", e.db);
    o.insert("ts", e.ts);
    o.insert("elapsedMs", e.elapsedMs);
    o.insert("rows", e.rows);
    o.insert("ok", e.ok);
    o.insert("error", e.error);
    return o;
}

HistoryEntry fromObj(const QJsonObject &o) {
    HistoryEntry e;
    e.sql = o.value("sql").toString();
    e.connId = o.value("connId").toString();
    e.db = o.value("db").toString();
    e.ts = (qint64)o.value("ts").toDouble();
    e.elapsedMs = o.value("elapsedMs").toInt();
    e.rows = o.value("rows").toInt();
    e.ok = o.value("ok").toBool(true);
    e.error = o.value("error").toString();
    return e;
}
}

void HistoryStore::append(const HistoryEntry &e) {
    QFile f(filePath());
    if (!f.open(QIODevice::Append | QIODevice::Text)) return;
    QTextStream ts(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ts.setCodec("UTF-8");
#endif
    ts << QString::fromUtf8(QJsonDocument(toObj(e)).toJson(QJsonDocument::Compact))
       << "\n";
}

QList<HistoryEntry> HistoryStore::load(int limit) {
    QList<HistoryEntry> out;
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return out;
    QList<HistoryEntry> all;
    QTextStream ts(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ts.setCodec("UTF-8");
#endif
    while (!ts.atEnd()) {
        const QString line = ts.readLine().trimmed();
        if (line.isEmpty()) continue;
        QJsonParseError err{};
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;
        all << fromObj(doc.object());
    }
    // newest first: reverse the file order (file is append=oldest first)
    for (int i = all.size() - 1; i >= 0 && out.size() < limit; --i)
        out << all.at(i);
    return out;
}

void HistoryStore::clear() {
    QFile::remove(filePath());
}

void HistoryStore::trim(int maxCount, int maxDays) {
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QList<HistoryEntry> all;
    {
        QTextStream ts(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        ts.setCodec("UTF-8");
#endif
        while (!ts.atEnd()) {
            const QString line = ts.readLine().trimmed();
            if (line.isEmpty()) continue;
            QJsonParseError err{};
            QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
            if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;
            all << fromObj(doc.object());
        }
    }
    f.close();

    if (maxDays > 0) {
        const qint64 cutoff =
            QDateTime::currentMSecsSinceEpoch() - (qint64)maxDays * 86400000LL;
        QList<HistoryEntry> kept;
        for (const HistoryEntry &e : all)
            if (e.ts >= cutoff) kept << e;
        all = kept;
    }
    if (maxCount > 0 && all.size() > maxCount)
        all = all.mid(all.size() - maxCount);  // keep tail (newest)

    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;
    QTextStream ts(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ts.setCodec("UTF-8");
#endif
    for (const HistoryEntry &e : all)
        ts << QString::fromUtf8(QJsonDocument(toObj(e)).toJson(QJsonDocument::Compact))
           << "\n";
}
