#include <QtTest>
#include "sql/SqlSplitter.h"

class TstSqlSplitter : public QObject {
    Q_OBJECT
private slots:
    void splitsTwoStatements() {
        QStringList r = SqlSplitter::split("select 1; select 2");
        QCOMPARE(r.size(), 2);
        QCOMPARE(r.at(0), QString("select 1"));
        QCOMPARE(r.at(1), QString("select 2"));
    }

    void ignoresSemicolonInString() {
        QStringList r = SqlSplitter::split("select ';' as a");
        QCOMPARE(r.size(), 1);
        QCOMPARE(r.at(0), QString("select ';' as a"));
    }

    void ignoresSemicolonInLineComment() {
        QStringList r = SqlSplitter::split("select 1 -- a ; b\n; select 2");
        QCOMPARE(r.size(), 2);
        QCOMPARE(r.at(1), QString("select 2"));
    }

    void ignoresSemicolonInBlockComment() {
        QStringList r = SqlSplitter::split("select /* ; */ 1; select 2");
        QCOMPARE(r.size(), 2);
        QCOMPARE(r.at(0), QString("select /* ; */ 1"));
    }

    void trailingSemicolonNoEmptyStatement() {
        QStringList r = SqlSplitter::split("select 1;   ");
        QCOMPARE(r.size(), 1);
        QCOMPARE(r.at(0), QString("select 1"));
    }

    void emptyScriptYieldsNothing() {
        QCOMPARE(SqlSplitter::split("   \n  ").size(), 0);
    }
};

QTEST_MAIN(TstSqlSplitter)
#include "tst_sqlsplitter.moc"
