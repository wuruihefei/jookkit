#include "sql/SqlFormat.h"
#include <QRegularExpression>
#include <QStringList>

QString SqlFormat::format(const QString &sql) {
    QString s = sql.trimmed();
    if (s.isEmpty()) return s;

    // 关键字大写
    static const QStringList kw = {
        "select","from","where","and","or","insert","into","values","update","set",
        "delete","create","table","drop","alter","add","column","index","view",
        "join","left","right","inner","outer","cross","on","group","by","order",
        "having","limit","offset","as","distinct","union","all","not","null","is",
        "in","like","between","case","when","then","else","end","asc","desc"
    };
    for (const QString &k : kw) {
        QRegularExpression re("\\b" + k + "\\b", QRegularExpression::CaseInsensitiveOption);
        s.replace(re, k.toUpper());
    }

    // JOIN(含修饰)前换行
    s.replace(QRegularExpression("\\s+((?:LEFT |RIGHT |INNER |OUTER |CROSS )?JOIN)\\b"),
              "\n\\1");
    // 主子句前换行
    const QStringList clauses = {"FROM", "WHERE", "GROUP BY", "ORDER BY",
                                 "HAVING", "LIMIT", "UNION"};
    for (const QString &c : clauses)
        s.replace(QRegularExpression("\\s+" + QRegularExpression::escape(c) + "\\b"),
                  "\n" + c);

    return s;
}
