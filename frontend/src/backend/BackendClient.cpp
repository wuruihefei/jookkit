#include "backend/BackendClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>

BackendClient::BackendClient(QObject *parent)
    : QObject(parent), nam(new QNetworkAccessManager(this)) {}

void BackendClient::setPort(int port) { port_ = port; }

BackendClient::Result BackendClient::call(const QJsonObject &request, int timeoutMs) {
    Result result;

    QUrl url(QString("http://127.0.0.1:%1/rpc").arg(port_));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray body = QJsonDocument(request).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = nam->post(req, body);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (!timer.isActive()) { // 超时
        result.transportFailed = true;
        result.errorCode = "TIMEOUT";
        result.errorMessage = "backend request timed out";
        reply->abort();
        reply->deleteLater();
        return result;
    }
    timer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        result.transportFailed = true;
        result.errorCode = "TRANSPORT";
        result.errorMessage = reply->errorString();
        reply->deleteLater();
        return result;
    }

    QByteArray resp = reply->readAll();
    reply->deleteLater();

    QJsonObject obj = QJsonDocument::fromJson(resp).object();
    result.ok = obj.value("ok").toBool();
    if (result.ok) {
        result.data = obj.value("data").toObject();
    } else {
        QJsonObject err = obj.value("error").toObject();
        result.errorCode = err.value("code").toString();
        result.errorMessage = err.value("message").toString();
    }
    return result;
}
