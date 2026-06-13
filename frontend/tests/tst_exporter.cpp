#include <QtTest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "export/Exporter.h"

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
};

#include "tst_exporter.moc"
