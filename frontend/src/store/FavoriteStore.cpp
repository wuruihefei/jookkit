#include "store/FavoriteStore.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

namespace {
QString filePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (dir.isEmpty()) dir = QDir::homePath() + "/.jookkit";
    QDir().mkpath(dir);
    return dir + "/favorites.json";
}
}

QList<Favorite> FavoriteStore::load() {
    QList<Favorite> out;
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly)) return out;
    const QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    for (const auto &v : arr) {
        const QJsonObject o = v.toObject();
        out << Favorite{o.value("connId").toString(),
                        o.value("db").toString(),
                        o.value("table").toString()};
    }
    return out;
}

void FavoriteStore::save(const QList<Favorite> &list) {
    QJsonArray arr;
    for (const Favorite &fav : list) {
        QJsonObject o;
        o.insert("connId", fav.connId);
        o.insert("db", fav.db);
        o.insert("table", fav.table);
        arr.append(o);
    }
    QFile f(filePath());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void FavoriteStore::add(const Favorite &fav) {
    QList<Favorite> list = load();
    if (list.contains(fav)) return;
    list << fav;
    save(list);
}

void FavoriteStore::remove(const Favorite &fav) {
    QList<Favorite> list = load();
    list.removeAll(fav);
    save(list);
}
