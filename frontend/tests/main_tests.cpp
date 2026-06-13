// 统一测试 main：依次运行所有测试类，汇总退出码。
// 增加新测试类时：在此文件中定义类，并在 main() 里追加一行
//   result |= QTest::qExec(new TstNewClass, argc, argv);
#include <QtTest>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTableWidget>
#include "backend/ConnData.h"
#include "backend/FuncId.h"
#include "export/Exporter.h"
#include "store/HistoryStore.h"
#include "ui/GridUtils.h"

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

    void insertMysqlPerRow() {
        QStringList headers{"id", "name"};
        QList<QStringList> rows{{"1", "A'B"}, {"2", "C"}};
        QByteArray out = Exporter::toInsertSql("t", headers, rows, "mysql", false);
        QCOMPARE(QString::fromUtf8(out),
            QString("INSERT INTO `t` (`id`, `name`) VALUES ('1', 'A''B');\n"
                    "INSERT INTO `t` (`id`, `name`) VALUES ('2', 'C');\n"));
    }

    void insertSqliteBatch() {
        QStringList headers{"id"};
        QList<QStringList> rows{{"1"}, {"2"}};
        QByteArray out = Exporter::toInsertSql("t", headers, rows, "sqlite", true);
        QCOMPARE(QString::fromUtf8(out),
            QString("INSERT INTO \"t\" (\"id\") VALUES ('1'), ('2');\n"));
    }

    void insertShortRow() {
        QStringList headers{"id", "name", "age"};
        QList<QStringList> rows{{"1", "Alice"}};  // missing "age" column
        QByteArray out = Exporter::toInsertSql("t", headers, rows, "mysql", false);
        // Should produce 3 values, not 2
        QCOMPARE(QString::fromUtf8(out),
            QString("INSERT INTO `t` (`id`, `name`, `age`) VALUES ('1', 'Alice', '');\n"));
    }
};

// ── TstHistoryStore ───────────────────────────────────────────
class TstHistoryStore : public QObject {
    Q_OBJECT
private slots:
    void init() {
        QStandardPaths::setTestModeEnabled(true);
        HistoryStore::clear();
    }

    void appendAndLoad() {
        HistoryEntry e;
        e.sql = "SELECT 1"; e.connId = "c1"; e.db = "d1";
        e.ts = 1000; e.elapsedMs = 5; e.rows = 1; e.ok = true;
        HistoryStore::append(e);

        QList<HistoryEntry> all = HistoryStore::load(100);
        QCOMPARE(all.size(), 1);
        QCOMPARE(all.at(0).sql, QString("SELECT 1"));
        QCOMPARE(all.at(0).rows, 1);
        QCOMPARE(all.at(0).ok, true);
    }

    void loadNewestFirst() {
        HistoryEntry a; a.sql = "A"; a.ts = 1;
        HistoryEntry b; b.sql = "B"; b.ts = 2;
        HistoryStore::append(a);
        HistoryStore::append(b);
        QList<HistoryEntry> all = HistoryStore::load(100);
        QCOMPARE(all.size(), 2);
        QCOMPARE(all.at(0).sql, QString("B"));  // newest first
        QCOMPARE(all.at(1).sql, QString("A"));
    }

    void trimByCount() {
        for (int i = 0; i < 5; ++i) {
            HistoryEntry e; e.sql = QString::number(i); e.ts = i;
            HistoryStore::append(e);
        }
        HistoryStore::trim(3, 0);
        QList<HistoryEntry> all = HistoryStore::load(100);
        QCOMPARE(all.size(), 3);
        QCOMPARE(all.at(0).sql, QString("4"));  // newest
        QCOMPARE(all.at(2).sql, QString("2"));
    }

    void trimByDays() {
        HistoryEntry oldE; oldE.sql = "old"; oldE.ts = 1;  // epoch 1ms = Jan 1 1970
        HistoryEntry newE; newE.sql = "new";
        newE.ts = QDateTime::currentMSecsSinceEpoch();
        HistoryStore::append(oldE);
        HistoryStore::append(newE);
        HistoryStore::trim(1000, 30);
        QList<HistoryEntry> all = HistoryStore::load(100);
        QCOMPARE(all.size(), 1);
        QCOMPARE(all.at(0).sql, QString("new"));
    }
};

// ── TstGridUtils ──────────────────────────────────────────────
class TstGridUtils : public QObject {
    Q_OBJECT
private slots:
    void extractVisibleOnly() {
        QTableWidget t;
        t.setColumnCount(2);
        t.setHorizontalHeaderLabels({"id", "name"});
        t.setRowCount(2);
        t.setItem(0, 0, new QTableWidgetItem("1"));
        t.setItem(0, 1, new QTableWidgetItem("Alice"));
        t.setItem(1, 0, new QTableWidgetItem("2"));
        t.setItem(1, 1, new QTableWidgetItem("Bob"));
        t.setRowHidden(1, true);

        QStringList headers;
        QList<QStringList> rows;
        GridUtils::extract(&t, true, false, headers, rows);
        QCOMPARE(headers, QStringList({"id", "name"}));
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows.at(0), QStringList({"1", "Alice"}));
    }
};

// ── runner ────────────────────────────────────────────────────
#include "main_tests.moc"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    int result = 0;
    { TstConnData t; result |= QTest::qExec(&t, argc, argv); }
    { TstExporter t; result |= QTest::qExec(&t, argc, argv); }
    { TstHistoryStore t; result |= QTest::qExec(&t, argc, argv); }
    { TstGridUtils t; result |= QTest::qExec(&t, argc, argv); }
    return result;
}
