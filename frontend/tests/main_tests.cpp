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
#include <QTextCodec>
#include "backend/ConnData.h"
#include "backend/FuncId.h"
#include "export/Exporter.h"
#include "store/HistoryStore.h"
#include "ui/GridUtils.h"
#include "import/CsvReader.h"
#include "import/JsonReader.h"
#include "import/ColumnMapping.h"
#include "import/ImportRunner.h"

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
        // 缺列 = QString()(null)= NULL,而非空串
        QCOMPARE(QString::fromUtf8(out),
            QString("INSERT INTO `t` (`id`, `name`, `age`) VALUES ('1', 'Alice', NULL);\n"));
    }

    // null QString → SQL NULL;空串 QString("") → '' ;普通值照旧转义
    void insertNullVsEmpty() {
        QStringList headers{"a", "b", "c"};
        QStringList row;
        row << QString() << QString("") << QString("O'Brien");
        QList<QStringList> rows{row};
        QByteArray out = Exporter::toInsertSql("t", headers, rows, "mysql", false);
        QCOMPARE(QString::fromUtf8(out),
            QString("INSERT INTO `t` (`a`, `b`, `c`) VALUES (NULL, '', 'O''Brien');\n"));
    }

    // batch 模式同样支持 NULL
    void insertBatchWithNull() {
        QStringList headers{"id", "v"};
        QList<QStringList> rows{{"1", QString()}, {"2", "ok"}};
        QByteArray out = Exporter::toInsertSql("t", headers, rows, "mysql", true);
        QCOMPARE(QString::fromUtf8(out),
            QString("INSERT INTO `t` (`id`, `v`) VALUES ('1', NULL), ('2', 'ok');\n"));
    }
};

// ── TstCsvReader ──────────────────────────────────────────────
class TstCsvReader : public QObject {
    Q_OBJECT
private slots:
    void basicHeaderAndRows() {
        QByteArray b = "id,name\r\n1,Alice\r\n2,Bob\r\n";
        ParseResult r = CsvReader::read(b, CsvOptions{});
        QVERIFY(r.ok);
        QCOMPARE(r.headers, QStringList({"id", "name"}));
        QCOMPARE(r.rows.size(), 2);
        QCOMPARE(r.rows.at(0), QStringList({"1", "Alice"}));
        QCOMPARE(r.rows.at(1), QStringList({"2", "Bob"}));
    }

    // 未加引号的空字段 → null;加引号的空字段 "" → 非 null 空串
    void emptyFieldIsNullQuotedEmptyIsNot() {
        QByteArray b = "a,b,c\n1,,3\n";
        ParseResult r = CsvReader::read(b, CsvOptions{});
        QVERIFY(r.ok);
        QCOMPARE(r.rows.at(0).size(), 3);
        QCOMPARE(r.rows.at(0).at(0), QString("1"));
        QVERIFY(r.rows.at(0).at(1).isNull());          // 空字段 = NULL
        QCOMPARE(r.rows.at(0).at(2), QString("3"));

        QByteArray b2 = "a,b\n\"\",x\n";                // 第一格是加引号空串
        ParseResult r2 = CsvReader::read(b2, CsvOptions{});
        QVERIFY(r2.rows.at(0).at(0).isEmpty());
        QVERIFY(!r2.rows.at(0).at(0).isNull());         // 引号空 = 空串非 NULL
        QCOMPARE(r2.rows.at(0).at(1), QString("x"));
    }

    void quotedCommaAndEscapedQuote() {
        QByteArray b = "h1,h2\n\"a,b\",\"c\"\"d\"\n";
        ParseResult r = CsvReader::read(b, CsvOptions{});
        QVERIFY(r.ok);
        QCOMPARE(r.rows.at(0).at(0), QString("a,b"));
        QCOMPARE(r.rows.at(0).at(1), QString("c\"d"));
    }

    void quotedEmbeddedNewline() {
        QByteArray b = "h\n\"line1\nline2\"\nr2\n";
        ParseResult r = CsvReader::read(b, CsvOptions{});
        QVERIFY(r.ok);
        QCOMPARE(r.rows.size(), 2);
        QCOMPARE(r.rows.at(0).at(0), QString("line1\nline2"));
        QCOMPARE(r.rows.at(1).at(0), QString("r2"));
    }

    void noHeader() {
        QByteArray b = "1,Alice\n2,Bob\n";
        CsvOptions o; o.firstRowHeader = false;
        ParseResult r = CsvReader::read(b, o);
        QVERIFY(r.headers.isEmpty());
        QCOMPARE(r.rows.size(), 2);
        QCOMPARE(r.rows.at(0), QStringList({"1", "Alice"}));
    }

    // sourceLines:记录起始物理行;字段内换行不增加“记录”但占物理行
    void sourceLineTracking() {
        QByteArray b = "h\n\"a\nb\"\nr2\n";
        ParseResult r = CsvReader::read(b, CsvOptions{});
        QCOMPARE(r.sourceLines.size(), 2);
        QCOMPARE(r.sourceLines.at(0), 2);   // 记录1从第2行开始(跨2-3行)
        QCOMPARE(r.sourceLines.at(1), 4);   // 记录2在第4行
    }

    void forcedGbk() {
        QTextCodec *gbk = QTextCodec::codecForName("GBK");
        QVERIFY(gbk != nullptr);
        QByteArray b = gbk->fromUnicode(QString::fromUtf8("名,值\n张三,1\n"));
        CsvOptions o; o.forcedCodec = "GBK";
        ParseResult r = CsvReader::read(b, o);
        QVERIFY(r.ok);
        QCOMPARE(r.detectedCodec, QString("GBK"));
        QCOMPARE(r.headers, QStringList({QString::fromUtf8("名"), QString::fromUtf8("值")}));
        QCOMPARE(r.rows.at(0), QStringList({QString::fromUtf8("张三"), "1"}));
    }

    void autoDetectUtf8Bom() {
        QByteArray b = QByteArray("\xEF\xBB\xBF") + QString::fromUtf8("名,值\n张三,1\n").toUtf8();
        ParseResult r = CsvReader::read(b, CsvOptions{});
        QVERIFY(r.ok);
        QCOMPARE(r.detectedCodec, QString("UTF-8"));
        QCOMPARE(r.headers, QStringList({QString::fromUtf8("名"), QString::fromUtf8("值")}));
    }

    void autoDetectGbkFallback() {
        QTextCodec *gbk = QTextCodec::codecForName("GBK");
        QByteArray b = gbk->fromUnicode(QString::fromUtf8("名,值\n张三,1\n"));
        ParseResult r = CsvReader::read(b, CsvOptions{});   // 无 forced,自动
        QVERIFY(r.ok);
        QCOMPARE(r.detectedCodec, QString("GBK"));           // 非法 UTF-8 → 回退 GBK
        QCOMPARE(r.rows.at(0).at(0), QString::fromUtf8("张三"));
    }

    void unterminatedQuoteIsError() {
        QByteArray b = "h\n\"abc\n";
        ParseResult r = CsvReader::read(b, CsvOptions{});
        QVERIFY(!r.ok);
        QVERIFY(!r.error.isEmpty());
    }
};

// ── TstJsonReader ─────────────────────────────────────────────
class TstJsonReader : public QObject {
    Q_OBJECT
private slots:
    void objectArrayBasic() {
        QByteArray b = "[{\"id\":1,\"name\":\"Alice\"}]";
        ParseResult r = JsonReader::read(b);
        QVERIFY(r.ok);
        QCOMPARE(r.rows.size(), 1);
        QCOMPARE(r.sourceLines.size(), r.rows.size());
        int idi = r.headers.indexOf("id"), ni = r.headers.indexOf("name");
        QVERIFY(idi >= 0 && ni >= 0);
        QCOMPARE(r.rows.at(0).at(idi), QString("1"));
        QCOMPARE(r.rows.at(0).at(ni), QString("Alice"));
    }

    void keyUnionAndMissing() {
        QByteArray b = "[{\"a\":1},{\"b\":2,\"a\":3}]";
        ParseResult r = JsonReader::read(b);
        QVERIFY(r.ok);
        QCOMPARE(r.headers.size(), 2);
        QVERIFY(r.headers.contains("a"));
        QVERIFY(r.headers.contains("b"));
        int ai = r.headers.indexOf("a"), bi = r.headers.indexOf("b");
        QCOMPARE(r.rows.size(), 2);
        QCOMPARE(r.rows.at(0).at(ai), QString("1"));
        QVERIFY(r.rows.at(0).at(bi).isNull());        // obj1 缺 b → NULL
        QCOMPARE(r.rows.at(1).at(ai), QString("3"));
        QCOMPARE(r.rows.at(1).at(bi), QString("2"));
    }

    void nullAndMissingAndEmpty() {
        QByteArray b = "[{\"a\":null,\"b\":\"\"},{}]";
        ParseResult r = JsonReader::read(b);
        QVERIFY(r.ok);
        int ai = r.headers.indexOf("a"), bi = r.headers.indexOf("b");
        QVERIFY(r.rows.at(0).at(ai).isNull());        // null → NULL
        QVERIFY(r.rows.at(0).at(bi).isEmpty());
        QVERIFY(!r.rows.at(0).at(bi).isNull());        // "" → 空串非 NULL
        QVERIFY(r.rows.at(1).at(ai).isNull());        // 缺键 → NULL
        QVERIFY(r.rows.at(1).at(bi).isNull());
    }

    void numbersAndBoolsTextified() {
        QByteArray b = "[{\"i\":123,\"f\":1.5,\"flag\":true}]";
        ParseResult r = JsonReader::read(b);
        QVERIFY(r.ok);
        int ii = r.headers.indexOf("i"), fi = r.headers.indexOf("f"), gi = r.headers.indexOf("flag");
        QCOMPARE(r.rows.at(0).at(ii), QString("123"));   // 整数不带 .0
        QCOMPARE(r.rows.at(0).at(fi), QString("1.5"));
        QCOMPARE(r.rows.at(0).at(gi), QString("true"));
    }

    void topLevelNotArrayIsError() {
        ParseResult r = JsonReader::read("{\"a\":1}");
        QVERIFY(!r.ok);
        QVERIFY(!r.error.isEmpty());
    }

    void elementNotObjectIsError() {
        ParseResult r = JsonReader::read("[1,2]");
        QVERIFY(!r.ok);
    }

    void invalidJsonIsError() {
        ParseResult r = JsonReader::read("not json");
        QVERIFY(!r.ok);
    }
};

// ── TstColumnMapping ──────────────────────────────────────────
class TstColumnMapping : public QObject {
    Q_OBJECT
private slots:
    void autoMatchExact() {
        QMap<QString, int> m = ColumnMapping::autoMatch({"id", "name"}, {"id", "name"});
        QCOMPARE(m.size(), 2);
        QCOMPARE(m.value("id"), 0);
        QCOMPARE(m.value("name"), 1);
    }

    void autoMatchCaseAndSpaceInsensitive() {
        QMap<QString, int> m = ColumnMapping::autoMatch({"  ID ", "Name"}, {"id", "name"});
        QCOMPARE(m.size(), 2);
        QCOMPARE(m.value("id"), 0);
        QCOMPARE(m.value("name"), 1);
    }

    void autoMatchPartial() {
        // 文件有 extra(表里没有)→ 忽略;表有 id(文件没有)→ 不映射
        QMap<QString, int> m = ColumnMapping::autoMatch({"name", "age", "extra"},
                                                        {"id", "name", "age"});
        QCOMPARE(m.size(), 2);
        QVERIFY(!m.contains("id"));
        QCOMPARE(m.value("name"), 0);
        QCOMPARE(m.value("age"), 1);
    }

    void applyOnlyMappedColsInTableOrder() {
        QStringList tableCols{"id", "name", "age"};
        QMap<QString, int> mapping; mapping["name"] = 0; mapping["age"] = 1;
        QList<QStringList> rows{{"Alice", "30"}, {"Bob", "25"}};
        QStringList outCols; QList<QStringList> outRows;
        ColumnMapping::apply(rows, tableCols, mapping, outCols, outRows);
        QCOMPARE(outCols, QStringList({"name", "age"}));   // id 未映射 → 不输出
        QCOMPARE(outRows.size(), 2);
        QCOMPARE(outRows.at(0), QStringList({"Alice", "30"}));
        QCOMPARE(outRows.at(1), QStringList({"Bob", "25"}));
    }

    void applyPreservesNullAndShortRow() {
        QStringList tableCols{"a", "b"};
        QMap<QString, int> mapping; mapping["a"] = 0; mapping["b"] = 1;
        QStringList r0; r0 << QString() << "x";            // a 为 null
        QStringList r1; r1 << "y";                          // 缺 index1
        QList<QStringList> rows{r0, r1};
        QStringList outCols; QList<QStringList> outRows;
        ColumnMapping::apply(rows, tableCols, mapping, outCols, outRows);
        QCOMPARE(outCols, QStringList({"a", "b"}));
        QVERIFY(outRows.at(0).at(0).isNull());             // null 透传
        QCOMPARE(outRows.at(0).at(1), QString("x"));
        QCOMPARE(outRows.at(1).at(0), QString("y"));
        QVERIFY(outRows.at(1).at(1).isNull());             // 越界 → NULL
    }
};

// ── TstImportRunner ───────────────────────────────────────────
class TstImportRunner : public QObject {
    Q_OBJECT
private:
    // 含 "BAD" 的 SQL 视为执行失败,否则成功
    static ImportRunner::Executor markerExec(int *calls) {
        return [calls](const QString &sql) -> ExecOutcome {
            if (calls) ++(*calls);
            if (sql.contains("BAD")) return ExecOutcome{false, "bad row"};
            return ExecOutcome{true, QString()};
        };
    }
private slots:
    void allSuccessUsesBatches() {
        QList<QStringList> rows{{"1","x"},{"2","y"},{"3","z"},{"4","w"},{"5","v"}};
        QVector<int> lines{2,3,4,5,6};
        int calls = 0;
        ImportResult r = ImportRunner::run("t", {"a","b"}, rows, "mysql", lines,
                                           markerExec(&calls), 2, nullptr);
        QCOMPARE(r.total, 5);
        QCOMPARE(r.success, 5);
        QVERIFY(r.failures.isEmpty());
        QCOMPARE(calls, 3);              // 批量:ceil(5/2)=3 次
        QVERIFY(!r.canceled);
    }

    void oneBadRowFallsBackPerRow() {
        QList<QStringList> rows{{"1","x"},{"2","y"},{"3","BAD"},{"4","z"}};
        QVector<int> lines{2,3,4,5};
        int calls = 0;
        ImportResult r = ImportRunner::run("t", {"a","b"}, rows, "mysql", lines,
                                           markerExec(&calls), 4, nullptr);
        QCOMPARE(r.total, 4);
        QCOMPARE(r.success, 3);          // 坏批回退后 3 行成功
        QCOMPARE(r.failures.size(), 1);
        QCOMPARE(r.failures.at(0).line, 4);                 // sourceLines[2]
        QCOMPARE(r.failures.at(0).data, QStringList({"3","BAD"}));
        QCOMPARE(calls, 5);             // 1 批 + 4 逐行
    }

    void wholeBatchAllBad() {
        QList<QStringList> rows{{"1","BAD"},{"2","BAD"}};
        QVector<int> lines{2,3};
        int calls = 0;
        ImportResult r = ImportRunner::run("t", {"a","b"}, rows, "mysql", lines,
                                           markerExec(&calls), 2, nullptr);
        QCOMPARE(r.success, 0);
        QCOMPARE(r.failures.size(), 2);
        QCOMPARE(calls, 3);             // 1 批 + 2 逐行
    }

    void cancelStopsEarly() {
        QList<QStringList> rows{{"1","a"},{"2","b"},{"3","c"},{"4","d"},{"5","e"},{"6","f"}};
        QVector<int> lines{2,3,4,5,6,7};
        int calls = 0;
        ImportRunner::Progress cancelNow = [](int, int){ return false; };
        ImportResult r = ImportRunner::run("t", {"a","b"}, rows, "mysql", lines,
                                           markerExec(&calls), 2, cancelNow);
        QVERIFY(r.canceled);
        QCOMPARE(r.success, 2);         // 仅第一批
        QCOMPARE(calls, 1);            // 取消后不再执行
        QCOMPARE(r.total, 6);
    }

    void batchSizeOneNoDoubleExec() {
        QList<QStringList> rows{{"1","x"},{"2","BAD"},{"3","z"}};
        QVector<int> lines{2,3,4};
        int calls = 0;
        ImportResult r = ImportRunner::run("t", {"a","b"}, rows, "mysql", lines,
                                           markerExec(&calls), 1, nullptr);
        QCOMPARE(r.success, 2);
        QCOMPARE(r.failures.size(), 1);
        QCOMPARE(r.failures.at(0).line, 3);
        QCOMPARE(calls, 3);             // 每行一次,坏行不二次执行
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
    { TstCsvReader t; result |= QTest::qExec(&t, argc, argv); }
    { TstJsonReader t; result |= QTest::qExec(&t, argc, argv); }
    { TstColumnMapping t; result |= QTest::qExec(&t, argc, argv); }
    { TstImportRunner t; result |= QTest::qExec(&t, argc, argv); }
    { TstHistoryStore t; result |= QTest::qExec(&t, argc, argv); }
    { TstGridUtils t; result |= QTest::qExec(&t, argc, argv); }
    return result;
}
