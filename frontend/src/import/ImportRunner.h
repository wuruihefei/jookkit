#ifndef JOOKKIT_IMPORTRUNNER_H
#define JOOKKIT_IMPORTRUNNER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>
#include <functional>

struct FailedRow {
    int         line;     // 源文件物理行(来自 sourceLines)
    QString     reason;   // 失败原因(执行错误文本)
    QStringList data;     // 该行原始单元格
};

struct ImportResult {
    int               total = 0;
    int               success = 0;
    QVector<FailedRow> failures;
    bool              canceled = false;
};

struct ExecOutcome { bool ok; QString error; };

// 批量 INSERT 执行;某批失败则对该批逐行重试以定位坏行。
// 不直接依赖 BackendClient:通过注入的 Executor 执行,便于单测。
namespace ImportRunner {
    using Executor = std::function<ExecOutcome(const QString &sql)>;
    using Progress = std::function<bool(int done, int total)>;   // 返回 false = 请求取消

    ImportResult run(const QString &table, const QString &db, const QStringList &cols,
                     const QList<QStringList> &rows, const QString &dbType,
                     const QVector<int> &sourceLines,
                     Executor exec, int batchSize, Progress onProgress);
}

#endif
