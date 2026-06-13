#ifndef JOOKKIT_EXPORTER_H
#define JOOKKIT_EXPORTER_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QList>

// 把表格数据导出为 CSV / JSON / SQL INSERT。纯函数,不依赖 UI。
// headers: 列名;rows: 每行单元格文本(长度=headers.size());单元格按文本处理。
namespace Exporter {
    QByteArray toCsv(const QStringList &headers, const QList<QStringList> &rows,
                     QChar sep, bool withHeader, bool bom);
    QByteArray toJson(const QStringList &headers, const QList<QStringList> &rows);
    QByteArray toInsertSql(const QString &table, const QStringList &headers,
                           const QList<QStringList> &rows,
                           const QString &dbType, bool batch);
}

#endif
