#ifndef JOOKKIT_FAVORITESTORE_H
#define JOOKKIT_FAVORITESTORE_H

#include <QString>
#include <QList>

// 一条收藏:某个连接下的某张表
struct Favorite {
    QString connId;
    QString db;
    QString table;

    bool operator==(const Favorite &o) const {
        return connId == o.connId && db == o.db && table == o.table;
    }
    QString display() const { return connId + " / " + db + " / " + table; }
};

// 收藏的表,持久化为配置目录下 favorites.json(与 connections.json 同目录)
namespace FavoriteStore {
    QList<Favorite> load();
    void save(const QList<Favorite> &list);
    void add(const Favorite &f);      // 已存在则忽略
    void remove(const Favorite &f);
}

#endif
