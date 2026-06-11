#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QLabel label("JookKit placeholder");
    label.show();
    return app.exec();
}
