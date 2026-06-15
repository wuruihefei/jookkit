#ifndef JOOKKIT_COLUMNMAPPING_H
#define JOOKKIT_COLUMNMAPPING_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

// 文件列 → 表列的映射与应用。
namespace ColumnMapping {

    // 按列名自动匹配(大小写/首尾空白不敏感)。
    // 返回:表列名 → 文件列下标(仅含匹配上的表列)。
    QMap<QString, int> autoMatch(const QStringList &fileHeaders, const QStringList &tableCols);

    // 按映射把文件行重排成“表列顺序”。
    // outCols:tableCols 中已映射的列(保持 tableCols 顺序);未映射的表列不输出。
    // outRows:逐行按 outCols 取值;文件侧越界 → QString()(NULL)。
    void apply(const QList<QStringList> &rows, const QStringList &tableCols,
               const QMap<QString, int> &mapping,
               QStringList &outCols, QList<QStringList> &outRows);
}

#endif
