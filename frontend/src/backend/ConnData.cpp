#include "backend/ConnData.h"
#include "backend/FuncId.h"

QJsonObject ConnData::toOpenRequest() const {
    QJsonObject o;
    o.insert("funcId", FuncId::OPEN_CONNECTION);
    o.insert("connId", connId);
    o.insert("type", type);
    if (type == "sqlite") {
        o.insert("file", file);
    } else {
        o.insert("host", host);
        o.insert("port", port);
        o.insert("user", user);
        o.insert("password", password);
        o.insert("database", database);
    }
    return o;
}
