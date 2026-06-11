#include "backend/BackendProcess.h"

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QFileInfo>

// 优先使用打包内置的 JRE(exe 同目录的 jre/bin/java[.exe]),否则回退到 PATH 中的 java。
static QString resolveJava() {
    const QString dir = QCoreApplication::applicationDirPath();
    const QStringList cands = {
        dir + "/jre/bin/java.exe",
        dir + "/jre/bin/java"
    };
    for (const QString &c : cands)
        if (QFileInfo::exists(c)) return c;
    return "java";
}

BackendProcess::BackendProcess(const QString &jarPath, QObject *parent)
    : QObject(parent), jarPath_(jarPath), proc_(new QProcess(this)) {
    proc_->setProcessChannelMode(QProcess::MergedChannels);
}

BackendProcess::~BackendProcess() { stop(); }

bool BackendProcess::start(int handshakeTimeoutMs) {
    proc_->start(resolveJava(), {"-jar", jarPath_, "--port", "0"});
    if (!proc_->waitForStarted(5000)) return false;

    QElapsedTimer timer;
    timer.start();
    QByteArray buf;
    QRegularExpression re("JOOKKIT_PORT=(\\d+)");
    while (timer.elapsed() < handshakeTimeoutMs) {
        if (proc_->waitForReadyRead(500)) {
            buf += proc_->readAll();
            auto m = re.match(QString::fromUtf8(buf));
            if (m.hasMatch()) {
                port_ = m.captured(1).toInt();
                return port_ > 0;
            }
        }
        if (proc_->state() == QProcess::NotRunning) return false;
    }
    return false;
}

void BackendProcess::stop() {
    if (proc_->state() != QProcess::NotRunning) {
        proc_->terminate();
        if (!proc_->waitForFinished(3000)) proc_->kill();
    }
}

bool BackendProcess::isRunning() const {
    return proc_->state() != QProcess::NotRunning;
}
