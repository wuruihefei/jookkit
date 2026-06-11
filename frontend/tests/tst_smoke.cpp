#include <QtTest>

class TstSmoke : public QObject {
    Q_OBJECT
private slots:
    void buildWorks() { QCOMPARE(2 + 2, 4); }
};

QTEST_MAIN(TstSmoke)
#include "tst_smoke.moc"
