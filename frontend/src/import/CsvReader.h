#ifndef JOOKKIT_CSVREADER_H
#define JOOKKIT_CSVREADER_H

#include <QByteArray>
#include <QString>
#include "import/ParseResult.h"

// CSV 解析(内存版)。字节流 → 表头 + 行。
// NULL 语义:未加引号的空字段 = QString()(isNull,代表 SQL NULL);
//            加引号的空字段 "" = QString("")(isEmpty 但非 null,代表空串)。
struct CsvOptions {
    QString forcedCodec;        // 空 = 自动检测;否则强制(如 "GBK")
    QChar   sep = ',';
    bool    firstRowHeader = true;
};

namespace CsvReader {
    ParseResult read(const QByteArray &bytes, const CsvOptions &opt);
}

#endif
