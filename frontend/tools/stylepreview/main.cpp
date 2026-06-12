// 一次性工具:离屏渲染「新建连接」式表单(同 light.qss),核对下拉框/数字框样式。
// 用法: QT_QPA_PLATFORM=offscreen ./stylepreview <输出前缀>
#include <QApplication>
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QFormLayout>
#include <QAbstractItemView>
#include <QFile>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QFile qss(":/resources/light.qss");
    if (qss.open(QFile::ReadOnly)) app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    QWidget form;
    form.resize(360, 220);
    auto *lay = new QFormLayout(&form);
    auto *name = new QLineEdit("conn1");
    auto *type = new QComboBox;
    type->addItems({"sqlite", "mysql"});
    type->setCurrentIndex(1);
    auto *host = new QLineEdit("127.0.0.1");
    auto *port = new QSpinBox;
    port->setRange(1, 65535); port->setValue(3306);
    auto *pageSize = new QComboBox;
    pageSize->addItems({"20", "100", "200", "500", "1000"});
    lay->addRow("名称:", name);
    lay->addRow("类型:", type);
    lay->addRow("主机:", host);
    lay->addRow("端口:", port);
    lay->addRow("每页:", pageSize);

    form.show();
    QString prefix = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : "style";
    form.grab().save(prefix + "-form.png");

    // 弹开下拉列表,截弹出视图
    pageSize->showPopup();
    QWidget *popup = pageSize->view()->window();
    popup->grab().save(prefix + "-popup.png");
    return 0;
}
