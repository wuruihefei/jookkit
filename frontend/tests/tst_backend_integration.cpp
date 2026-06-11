#include <QtTest>
#include <QProcessEnvironment>
#include <QJsonObject>
#include <QJsonArray>
#include "backend/BackendProcess.h"
#include "backend/BackendClient.h"
#include "backend/ConnData.h"
#include "backend/FuncId.h"

class TstBackendIntegration : public QObject {
    Q_OBJECT
    QString jar;
private slots:
    void initTestCase() {
        jar = QProcessEnvironment::systemEnvironment().value("JOOKKIT_JAR");
        if (jar.isEmpty()) QSKIP("set JOOKKIT_JAR to run integration test");
    }

    void openAndQuerySqlite() {
        BackendProcess proc(jar);
        QVERIFY2(proc.start(), "backend failed to start / handshake");
        QVERIFY(proc.port() > 0);

        BackendClient client;
        client.setPort(proc.port());

        ConnData c;
        c.connId = "c1"; c.type = "sqlite"; c.file = ":memory:";
        auto r1 = client.call(c.toOpenRequest());
        QVERIFY2(r1.ok, qPrintable(r1.errorMessage));

        QJsonObject exec;
        exec.insert("funcId", FuncId::EXEC_SQL);
        exec.insert("connId", "c1");
        exec.insert("sql", "select 99 as n");
        auto r2 = client.call(exec);
        QVERIFY2(r2.ok, qPrintable(r2.errorMessage));
        int v = r2.data.value("rows").toArray()
                  .at(0).toArray().at(0).toInt();
        QCOMPARE(v, 99);
    }

    void sqlErrorSurfaces() {
        BackendProcess proc(jar);
        QVERIFY(proc.start());
        BackendClient client;
        client.setPort(proc.port());
        ConnData c; c.connId = "c1"; c.type = "sqlite"; c.file = ":memory:";
        client.call(c.toOpenRequest());

        QJsonObject exec;
        exec.insert("funcId", FuncId::EXEC_SQL);
        exec.insert("connId", "c1");
        exec.insert("sql", "select * from nope");
        auto r = client.call(exec);
        QVERIFY(!r.ok);
        QCOMPARE(r.errorCode, QString("SQL_ERROR"));
    }
};

QTEST_MAIN(TstBackendIntegration)
#include "tst_backend_integration.moc"
