#ifndef JOOKKIT_BACKENDCLIENT_H
#define JOOKKIT_BACKENDCLIENT_H

#include <QObject>
#include <QJsonObject>
#include <QString>

class QNetworkAccessManager;

class BackendClient : public QObject {
    Q_OBJECT
public:
    struct Result {
        bool ok = false;
        QJsonObject data;
        QString errorCode;
        QString errorMessage;
        bool transportFailed = false;  // 网络层失败(非业务错误)
    };

    explicit BackendClient(QObject *parent = nullptr);
    void setPort(int port);
    void setHistoryContext(const QString &connId, const QString &db);

    /** 阻塞式调用 /rpc,超时返回 transportFailed。 */
    Result call(const QJsonObject &request, int timeoutMs = 30000);

private:
    QNetworkAccessManager *nam;
    int port_ = 0;
    QString histConnId_;
    QString histDb_;
};

#endif
