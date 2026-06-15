#include "import/ColumnMapping.h"
#include <QHash>
#include <QVector>

namespace {
QString norm(const QString &s) { return s.trimmed().toLower(); }
}

namespace ColumnMapping {

QMap<QString, int> autoMatch(const QStringList &fileHeaders, const QStringList &tableCols) {
    // 规范化文件列名 → 首个下标
    QHash<QString, int> fileIndex;
    for (int i = 0; i < fileHeaders.size(); ++i) {
        const QString k = norm(fileHeaders.at(i));
        if (!fileIndex.contains(k)) fileIndex.insert(k, i);
    }
    QMap<QString, int> out;
    for (const QString &col : tableCols) {
        auto it = fileIndex.constFind(norm(col));
        if (it != fileIndex.constEnd()) out.insert(col, it.value());
    }
    return out;
}

void apply(const QList<QStringList> &rows, const QStringList &tableCols,
           const QMap<QString, int> &mapping,
           QStringList &outCols, QList<QStringList> &outRows) {
    outCols.clear();
    outRows.clear();
    // 输出列 = tableCols 中已映射的列(保持顺序);并记录对应文件下标
    QVector<int> srcIdx;
    for (const QString &col : tableCols) {
        auto it = mapping.constFind(col);
        if (it != mapping.constEnd()) { outCols << col; srcIdx << it.value(); }
    }
    for (const QStringList &row : rows) {
        QStringList outRow;
        for (int idx : srcIdx)
            outRow << (idx >= 0 && idx < row.size() ? row.at(idx) : QString());  // 越界 → NULL
        outRows << outRow;
    }
}

} // namespace ColumnMapping
