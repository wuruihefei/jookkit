#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QPixmap>
#include <QPainter>
#include <QIcon>
#include <QFont>
#include <QMetaObject>
#include <QFileInfo>
#include <csignal>
#include "ui/MainWindow.h"

// 收到 SIGINT/SIGTERM 时排队触发 Qt 正常退出,使 BackendProcess::stop() 得以运行。
static void handleSignal(int) {
    if (qApp) QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
}

// 按多个候选位置定位后端 jar:环境变量优先,其次打包布局,最后开发布局。
static QString resolveJar() {
    QString env = qEnvironmentVariable("JOOKKIT_JAR");
    if (!env.isEmpty() && QFileInfo::exists(env)) return env;
    const QString dir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        dir + "/jookkit-backend.jar",
        dir + "/backend/jookkit-backend.jar",
        dir + "/../backend/target/jookkit-backend.jar"
    };
    for (const QString &c : candidates)
        if (QFileInfo::exists(c)) return c;
    return env.isEmpty() ? candidates.last() : env;
}

// 程序化生成应用图标(圆角蓝底白色 J),免外部 .ico/.png。
static QIcon appIcon() {
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#2563eb"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(6, 6, 52, 52, 12, 12);
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    f.setPixelSize(34);
    p.setFont(f);
    p.drawText(QRect(6, 6, 52, 52), Qt::AlignCenter, "J");
    p.end();
    return QIcon(pm);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    app.setStyle(QStyleFactory::create("Fusion"));
    QFile qss(":/resources/light.qss");
    if (qss.open(QFile::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
        qss.close();
    }
    app.setWindowIcon(appIcon());

    MainWindow w(resolveJar());
    w.show();
    return app.exec();
}
