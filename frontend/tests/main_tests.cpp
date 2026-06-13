// 统一测试 main：依次运行所有测试类，汇总退出码。
// 增加新测试类时：在此文件中定义类并加入 runAll()。
#include <QtTest>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "backend/ConnData.h"
#include "backend/FuncId.h"
#include "export/Exporter.h"

// ── TstConnData ───────────────────────────────────────────────
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

    void testRequestUsesTestFuncId() {
        ConnData c;
        c.connId = "c1";
        c.type = "sqlite";
        c.file = "/tmp/x.db";
        QJsonObject req = c.toTestRequest();
        QCOMPARE(req.value("funcId").toInt(), FuncId::TEST_CONNECTION);
        QCOMPARE(req.value("file").toString(), QString("/tmp/x.db"));
    }
};

// ── TstExporter ───────────────────────────────────────────────
class TstExporter : public QObject {
    Q_OBJECT
private slots:
    void csvBasic() {
        QStringList headers{"id", "name"};
        QList<QStringList> rows{{"1", "Alice"}, {"2", "Bob"}};
        QByteArray out = Exporter::toCsv(headers, rows, ',', true, false);
        QCOMPARE(QString::fromUtf8(out), QString("id,name\r\n1,Alice\r\n2,Bob\r\n"));
    }

    void csvQuotingAndBom() {
        QStringList headers{"a", "b"};
        QList<QStringList> rows{{"x,y", "he said \"hi\""}, {"line1\nline2", "ok"}};
        QByteArray out = Exporter::toCsv(headers, rows, ',', true, true);
        QVERIFY(out.startsWith("\xEF\xBB\xBF"));
        QString body = QString::fromUtf8(out.mid(3));
        QCOMPARE(body,
            QString("a,b\r\n\"x,y\",\"he said \"\"hi\"\"\"\r\n\"line1\nline2\",ok\r\n"));
    }

    void csvNoHeader() {
        QStringList headers{"id"};
        QList<QStringList> rows{{"1"}};
        QByteArray out = Exporter::toCsv(headers, rows, '\t', false, false);
        QCOMPARE(QString::fromUtf8(out), QString("1\r\n"));
    }

    void jsonBasic() {
        QStringList headers{"id", "name"};
        QList<QStringList> rows{{"1", "Alice"}};
        QByteArray out = Exporter::toJson(headers, rows);
        QJsonDocument doc = QJsonDocument::fromJson(out);
        QVERIFY(doc.isArray());
        QJsonArray arr = doc.array();
        QCOMPARE(arr.size(), 1);
        QCOMPARE(arr.at(0).toObject().value("id").toString(), QString("1"));
        QCOMPARE(arr.at(0).toObject().value("name").toString(), QString("Alice"));
    }
};

// ── runner ────────────────────────────────────────────────────
#include "main_tests.moc"

int main(int argc, char **argv) {
    int result = 0;
    { TstConnData t; result |= QTest::qExec(&t, argc, argv); }
    { TstExporter t; result |= QTest::qExec(&t, argc, argv); }
    return result;
}
