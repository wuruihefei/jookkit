#include <QCoreApplication>
#include <QVariant>
#include <QDebug>
#include "backend/ConnData.h"
#include "store/ConnectionStore.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("JookKitTest");
    QCoreApplication::setApplicationName("JookKitTest");

    ConnData c;
    c.connId = "s1"; c.type = "sqlite"; c.file = "/tmp/x.db";

    QVariant v = QVariant::fromValue(c);
    qDebug() << "canConvert<ConnData>:" << v.canConvert<ConnData>();
    ConnData back = v.value<ConnData>();
    qDebug() << "roundtrip:" << back.connId << back.type << back.file;

    ConnectionStore::save({c});
    auto loaded = ConnectionStore::load();
    qDebug() << "loaded count:" << loaded.size();
    for (const auto &x : loaded)
        qDebug() << "  loaded:" << x.connId << x.type << x.file;
    return 0;
}
