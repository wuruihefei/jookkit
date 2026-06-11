#include "store/ConnectionStore.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace {
const char kKey[] = "JookKit-local-obfuscation-key-v1";
}

QString ConnectionStore::filePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (dir.isEmpty()) dir = QDir::homePath() + "/.jookkit";
    QDir().mkpath(dir);
    return dir + "/connections.json";
}

// 简单 XOR + Base64 混淆(仅防肩窥,非安全加密)。
QString ConnectionStore::obfuscate(const QString &plain) {
    QByteArray data = plain.toUtf8();
    QByteArray key(kKey);
    for (int i = 0; i < data.size(); ++i)
        data[i] = data[i] ^ key[i % key.size()];
    return QString::fromLatin1(data.toBase64());
}

QString ConnectionStore::deobfuscate(const QString &enc) {
    QByteArray data = QByteArray::fromBase64(enc.toLatin1());
    QByteArray key(kKey);
    for (int i = 0; i < data.size(); ++i)
        data[i] = data[i] ^ key[i % key.size()];
    return QString::fromUtf8(data);
}

QList<ConnData> ConnectionStore::load() {
    QList<ConnData> out;
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly)) return out;
    QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    f.close();
    for (const auto &v : arr) {
        QJsonObject o = v.toObject();
        ConnData c;
        c.connId = o.value("connId").toString();
        c.type = o.value("type").toString();
        c.host = o.value("host").toString();
        c.port = o.value("port").toInt();
        c.user = o.value("user").toString();
        c.password = deobfuscate(o.value("password").toString());
        c.database = o.value("database").toString();
        c.file = o.value("file").toString();
        c.params = o.value("params").toString();
        out << c;
    }
    return out;
}

void ConnectionStore::save(const QList<ConnData> &conns) {
    QJsonArray arr;
    for (const ConnData &c : conns) {
        QJsonObject o;
        o.insert("connId", c.connId);
        o.insert("type", c.type);
        o.insert("host", c.host);
        o.insert("port", c.port);
        o.insert("user", c.user);
        o.insert("password", obfuscate(c.password));
        o.insert("database", c.database);
        o.insert("file", c.file);
        o.insert("params", c.params);
        arr.append(o);
    }
    QFile f(filePath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        f.close();
    }
}
