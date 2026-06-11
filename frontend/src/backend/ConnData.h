#ifndef JOOKKIT_CONNDATA_H
#define JOOKKIT_CONNDATA_H

#include <QString>
#include <QJsonObject>

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

    QJsonObject toOpenRequest() const;
};

#endif
