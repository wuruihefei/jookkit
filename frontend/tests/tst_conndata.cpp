#include <QtTest>
#include <QJsonObject>
#include "backend/ConnData.h"
#include "backend/FuncId.h"

class TstConnData : public QObject {
    Q_OBJECT
private slots:
    void sqliteOpenRequest() {
        ConnData c;
        c.connId = "c1";
        c.type = "sqlite";
        c.file = "/tmp/x.db";
        QJsonObject req = c.toOpenRequest();
        QCOMPARE(req.value("funcId").toInt(), FuncId::OPEN_CONNECTION);
        QCOMPARE(req.value("connId").toString(), QString("c1"));
        QCOMPARE(req.value("type").toString(), QString("sqlite"));
        QCOMPARE(req.value("file").toString(), QString("/tmp/x.db"));
    }

    void mysqlOpenRequest() {
        ConnData c;
        c.connId = "m1";
        c.type = "mysql";
        c.host = "localhost";
        c.port = 3306;
        c.user = "root";
        c.database = "test";
        QJsonObject req = c.toOpenRequest();
        QCOMPARE(req.value("host").toString(), QString("localhost"));
        QCOMPARE(req.value("port").toInt(), 3306);
        QCOMPARE(req.value("user").toString(), QString("root"));
        QCOMPARE(req.value("database").toString(), QString("test"));
    }
};

QTEST_MAIN(TstConnData)
#include "tst_conndata.moc"
