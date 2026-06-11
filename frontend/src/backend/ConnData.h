#ifndef JOOKKIT_CONNDATA_H
#define JOOKKIT_CONNDATA_H

#include <QString>
#include <QJsonObject>
#include <QMetaType>

class ConnData {
public:
    QString connId;
    QString type;       // "mysql" | "sqlite"
    QString host;
    int port = 0;
    QString user;
    QString password;
    QString database;
    QString file;       // sqlite
    QString params;     // mysql 可选附加参数

    QJsonObject toOpenRequest() const;
    QJsonObject toTestRequest() const;

private:
    QJsonObject toRequest(int funcId) const;
};

Q_DECLARE_METATYPE(ConnData)

#endif
