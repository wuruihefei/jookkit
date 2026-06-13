#include "backend/BackendClient.h"
#include "backend/FuncId.h"
#include "store/HistoryStore.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QElapsedTimer>
#include <QDateTime>

BackendClient::BackendClient(QObject *parent)
    : QObject(parent), nam(new QNetworkAccessManager(this)) {}

void BackendClient::setPort(int port) { port_ = port; }

void BackendClient::setHistoryContext(const QString &connId, const QString &db) {
    histConnId_ = connId;
    histDb_ = db;
}

BackendClient::Result BackendClient::call(const QJsonObject &request, int timeoutMs) {
    Result result;

    QElapsedTimer _histTimer;
    _histTimer.start();
    const int _funcId = request.value("funcId").toInt();
    const bool _isExecClass = (_funcId == FuncId::EXEC_SQL  ||
                                _funcId == FuncId::INSERT_ROW ||
                                _funcId == FuncId::UPDATE_ROW ||
                                _funcId == FuncId::DELETE_ROW);

    auto _recordHistory = [&](const Result &r) {
        if (!_isExecClass) return;
        // skip internal USE statements (database switch, not user SQL)
        const QString _sql = request.value("sql").toString();
        if (_sql.trimmed().startsWith("USE ", Qt::CaseInsensitive)) return;
        HistoryEntry _h;
        // SQL text: use "sql" field for EXEC_SQL; synthesize for DML operations
        _h.sql = request.value("sql").toString();
        if (_h.sql.isEmpty()) {
            const QString tbl = request.value("db").toString() + "." +
                                request.value("table").toString();
            if (_funcId == FuncId::INSERT_ROW)      _h.sql = "[INSERT " + tbl + "]";
            else if (_funcId == FuncId::UPDATE_ROW) _h.sql = "[UPDATE " + tbl + "]";
            else if (_funcId == FuncId::DELETE_ROW) _h.sql = "[DELETE " + tbl + "]";
        }
        // connId/db: prefer from request (TableDataForm includes them); fall back to injected context
        _h.connId = request.value("connId").toString();
        if (_h.connId.isEmpty()) _h.connId = histConnId_;
        _h.db = request.value("db").toString();
        if (_h.db.isEmpty()) _h.db = histDb_;
        _h.ts = QDateTime::currentMSecsSinceEpoch();
        _h.elapsedMs = (int)_histTimer.elapsed();
        _h.ok = r.ok;
        _h.error = r.ok ? QString()
                        : (r.errorMessage.isEmpty() ? r.errorCode : r.errorMessage);
        // rows: SELECT returns data["rows"] array; DML returns data["affected"]
        if (r.data.value("rows").isArray())
            _h.rows = r.data.value("rows").toArray().size();
        else
            _h.rows = r.data.value("affected").toInt();
        HistoryStore::append(_h);
    };

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
        _recordHistory(result);
        return result;
    }
    timer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        result.transportFailed = true;
        result.errorCode = "TRANSPORT";
        result.errorMessage = reply->errorString();
        reply->deleteLater();
        _recordHistory(result);
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
    _recordHistory(result);
    return result;
}
