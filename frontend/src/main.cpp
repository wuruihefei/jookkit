#include <QApplication>
#include <QMetaObject>
#include <csignal>
#include "ui/MainWindow.h"

// 收到 SIGINT/SIGTERM 时排队触发 Qt 正常退出,使 MainWindow 析构、
// BackendProcess::stop() 得以运行,避免后端子进程被孤儿化。
static void handleSignal(int) {
    if (qApp) QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    // jar 路径:优先环境变量,否则相对默认位置
    QString jar = qEnvironmentVariable("JOOKKIT_JAR",
        QCoreApplication::applicationDirPath()
        + "/../backend/target/jookkit-backend.jar");
    MainWindow w(jar);
    w.show();
    return app.exec();
}
