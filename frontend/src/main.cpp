#include <QApplication>
#include "ui/MinimalWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    // jar 路径:优先环境变量,否则相对默认位置
    QString jar = qEnvironmentVariable("JOOKKIT_JAR",
        QCoreApplication::applicationDirPath()
        + "/../backend/target/jookkit-backend.jar");
    MinimalWindow w(jar);
    w.show();
    return app.exec();
}
