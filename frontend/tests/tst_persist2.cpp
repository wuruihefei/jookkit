#include <QApplication>
#include <QDebug>
#include <QTemporaryFile>
#include "ui/ObjectTree.h"
#include "backend/BackendProcess.h"
#include "backend/BackendClient.h"
#include "backend/ConnData.h"
#include "store/ConnectionStore.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("JookKitTest2");
    QCoreApplication::setApplicationName("JookKitTest2");

    QString jar = qEnvironmentVariable("JOOKKIT_JAR");
    if (jar.isEmpty()) { qDebug() << "set JOOKKIT_JAR"; return 2; }

    BackendProcess proc(jar);
    if (!proc.start()) { qDebug() << "backend start failed"; return 3; }
    BackendClient client; client.setPort(proc.port());

    ObjectTree tree(&client);
    ConnData c;
    c.connId = "sqlitefile";
    c.type = "sqlite";
    c.file = "/tmp/jk_persist_test.db";

    bool added = tree.addConnection(c);
    qDebug() << "addConnection returned:" << added;

    auto all = tree.allConnections();
    qDebug() << "allConnections size:" << all.size();
    for (const auto &x : all)
        qDebug() << "  conn:" << x.connId << x.type << x.file;

    ConnectionStore::save(all);
    auto loaded = ConnectionStore::load();
    qDebug() << "reloaded size:" << loaded.size();
    for (const auto &x : loaded)
        qDebug() << "  reloaded:" << x.connId << x.type << x.file;
    return 0;
}
