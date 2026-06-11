#ifndef JOOKKIT_SQLSPLITTER_H
#define JOOKKIT_SQLSPLITTER_H

#include <QString>
#include <QStringList>

// 把 SQL 脚本按分号切分成多条语句,正确跳过:
//  - 单引号 '...' 和双引号 "..." 字符串字面量中的分号
//  - 行注释 -- ... 到行尾
//  - 块注释 /* ... */
// 返回去除首尾空白后的非空语句列表。
class SqlSplitter {
public:
    static QStringList split(const QString &script);
};

#endif
