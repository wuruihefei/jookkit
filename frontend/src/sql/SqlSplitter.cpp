#include "sql/SqlSplitter.h"

QStringList SqlSplitter::split(const QString &script) {
    QStringList out;
    QString cur;
    enum State { Normal, Single, Double, LineComment, BlockComment };
    State st = Normal;

    const int n = script.size();
    for (int i = 0; i < n; ++i) {
        QChar c = script.at(i);
        QChar next = (i + 1 < n) ? script.at(i + 1) : QChar();

        switch (st) {
        case Normal:
            if (c == '\'') { st = Single; cur += c; }
            else if (c == '"') { st = Double; cur += c; }
            else if (c == '-' && next == '-') { st = LineComment; cur += c; }
            else if (c == '/' && next == '*') { st = BlockComment; cur += c; }
            else if (c == ';') {
                QString stmt = cur.trimmed();
                if (!stmt.isEmpty()) out << stmt;
                cur.clear();
            } else {
                cur += c;
            }
            break;
        case Single:
            cur += c;
            if (c == '\'') st = Normal;
            break;
        case Double:
            cur += c;
            if (c == '"') st = Normal;
            break;
        case LineComment:
            cur += c;
            if (c == '\n') st = Normal;
            break;
        case BlockComment:
            cur += c;
            if (c == '*' && next == '/') { cur += next; ++i; st = Normal; }
            break;
        }
    }

    QString tail = cur.trimmed();
    if (!tail.isEmpty()) out << tail;
    return out;
}
