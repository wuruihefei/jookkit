#ifndef JOOKKIT_HISTORYSTORE_H
#define JOOKKIT_HISTORYSTORE_H

#include <QString>
#include <QList>

struct HistoryEntry {
    QString sql;
    QString connId;
    QString db;
    qint64  ts = 0;        // epoch ms
    int     elapsedMs = 0;
    int     rows = 0;
    bool    ok = true;
    QString error;
};

namespace HistoryStore {
    void append(const HistoryEntry &e);
    QList<HistoryEntry> load(int limit);   // newest first
    void clear();
    void trim(int maxCount, int maxDays);  // maxDays<=0 means no day-based trimming
}

#endif
