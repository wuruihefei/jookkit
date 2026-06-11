#ifndef JOOKKIT_SQLFORMAT_H
#define JOOKKIT_SQLFORMAT_H

#include <QString>

// 轻量 SQL 格式化:关键字大写 + 主子句换行(非完整解析,够日常用)。
namespace SqlFormat {
    QString format(const QString &sql);
}

#endif
