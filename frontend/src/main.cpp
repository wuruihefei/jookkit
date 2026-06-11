#include <QApplication>
#include <QMetaObject>
#include <QFileInfo>
#include <csignal>
#include "ui/MainWindow.h"

// 收到 SIGINT/SIGTERM 时排队触发 Qt 正常退出,使 MainWindow 析构、
// BackendProcess::stop() 得以运行,避免后端子进程被孤儿化。
static void handleSignal(int) {
    if (qApp) QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
}

// 按多个候选位置定位后端 jar:环境变量优先,其次打包布局,最后开发布局。
static QString resolveJar() {
    QString env = qEnvironmentVariable("JOOKKIT_JAR");
    if (!env.isEmpty() && QFileInfo::exists(env)) return env;
    const QString dir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        dir + "/jookkit-backend.jar",                 // jar 与 exe 同目录
        dir + "/backend/jookkit-backend.jar",         // 打包布局
        dir + "/../backend/target/jookkit-backend.jar" // 开发布局
    };
    for (const QString &c : candidates)
        if (QFileInfo::exists(c)) return c;
    return env.isEmpty() ? candidates.last() : env;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    MainWindow w(resolveJar());
    w.show();
    return app.exec();
}
