#include "backend/ConnData.h"
#include "backend/FuncId.h"

QJsonObject ConnData::toRequest(int funcId) const {
    QJsonObject o;
    o.insert("funcId", funcId);
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

QJsonObject ConnData::toOpenRequest() const {
    return toRequest(FuncId::OPEN_CONNECTION);
}

QJsonObject ConnData::toTestRequest() const {
    return toRequest(FuncId::TEST_CONNECTION);
}
