#ifndef JOOKKIT_CONNECTIONSTORE_H
#define JOOKKIT_CONNECTIONSTORE_H

#include <QList>
#include "backend/ConnData.h"

// 连接配置本地持久化(JSON 文件存于用户配置目录;密码做轻量混淆,非强加密)。
class ConnectionStore {
public:
    static QList<ConnData> load();
    static void save(const QList<ConnData> &conns);

private:
    static QString filePath();
    static QString obfuscate(const QString &plain);
    static QString deobfuscate(const QString &enc);
};

#endif
