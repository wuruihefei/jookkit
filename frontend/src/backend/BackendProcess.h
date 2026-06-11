#ifndef JOOKKIT_BACKENDPROCESS_H
#define JOOKKIT_BACKENDPROCESS_H

#include <QObject>
#include <QProcess>
#include <QString>

class BackendProcess : public QObject {
    Q_OBJECT
public:
    explicit BackendProcess(const QString &jarPath, QObject *parent = nullptr);
    ~BackendProcess() override;

    /** 启动并阻塞等待握手(读到 JOOKKIT_PORT)。成功返回 true。 */
    bool start(int handshakeTimeoutMs = 15000);
    void stop();
    int port() const { return port_; }
    bool isRunning() const;

private:
    QString jarPath_;
    QProcess *proc_;
    int port_ = 0;
};

#endif
