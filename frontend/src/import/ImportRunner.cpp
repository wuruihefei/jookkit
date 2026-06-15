#include "import/ImportRunner.h"
#include "export/Exporter.h"
#include <QtGlobal>

namespace ImportRunner {

ImportResult run(const QString &table, const QStringList &cols,
                 const QList<QStringList> &rows, const QString &dbType,
                 const QVector<int> &sourceLines,
                 Executor exec, int batchSize, Progress onProgress) {
    ImportResult res;
    res.total = rows.size();
    const int bs = qMax(1, batchSize);

    int i = 0;
    while (i < rows.size()) {
        const int end = qMin(i + bs, rows.size());
        const QList<QStringList> slice = rows.mid(i, end - i);

        const QString batchSql =
            QString::fromUtf8(Exporter::toInsertSql(table, cols, slice, dbType, /*batch=*/true));
        const ExecOutcome o = exec(batchSql);

        if (o.ok) {
            res.success += slice.size();
        } else if (slice.size() == 1) {
            // 单行批失败:直接记录,不二次执行
            res.failures.append(FailedRow{ sourceLines.value(i, i + 1), o.error, slice.at(0) });
        } else {
            // 批失败回退逐行,定位坏行;成功行照常计入
            for (int j = 0; j < slice.size(); ++j) {
                const QString oneSql = QString::fromUtf8(
                    Exporter::toInsertSql(table, cols, { slice.at(j) }, dbType, /*batch=*/false));
                const ExecOutcome oo = exec(oneSql);
                if (oo.ok)
                    res.success++;
                else
                    res.failures.append(FailedRow{ sourceLines.value(i + j, i + j + 1),
                                                   oo.error, slice.at(j) });
            }
        }

        i = end;
        if (onProgress && !onProgress(i, res.total)) { res.canceled = true; break; }
    }
    return res;
}

} // namespace ImportRunner
