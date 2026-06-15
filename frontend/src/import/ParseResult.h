#ifndef JOOKKIT_PARSERESULT_H
#define JOOKKIT_PARSERESULT_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>

// CSV/JSON 解析的统一结果。
// NULL 语义:单元格 QString()(isNull)= SQL NULL;QString("")(空串非 null)= 空字符串。
struct ParseResult {
    QStringList         headers;       // 无表头时为空
    QList<QStringList>  rows;          // 每格:QString() = NULL
    QVector<int>        sourceLines;   // 与 rows 等长,每条记录起始物理行(1-based)
    QString             detectedCodec; // 实际使用的编码名,如 "UTF-8"/"GBK"
    bool                ok = true;
    QString             error;
};

#endif
