// 一次性工具:离屏渲染对象树样例(同 light.qss + Icons),输出 PNG 供人工核对。
// 用法: QT_QPA_PLATFORM=offscreen ./treepreview <输出.png>
#include <QApplication>
#include <QTreeWidget>
#include <QFile>
#include "ui/Icons.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QFile qss(":/resources/light.qss");
    if (qss.open(QFile::ReadOnly)) app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    QTreeWidget tree;
    tree.setHeaderHidden(true);
    tree.setIconSize(QSize(18, 18));  // 与 ObjectTree 保持一致
    tree.resize(300, 420);

    // 已连接 mysql:展开,含数据库与表
    auto *open1 = new QTreeWidgetItem(&tree, QStringList("conn-mysql(已连接)"));
    open1->setIcon(0, Icons::connection("mysql", true));
    auto *db1 = new QTreeWidgetItem(open1, QStringList("LLM_APP_SERVICE"));
    db1->setIcon(0, Icons::database());
    auto *t1 = new QTreeWidgetItem(db1, QStringList("user_info"));
    t1->setIcon(0, Icons::table());
    auto *t2 = new QTreeWidgetItem(db1, QStringList("order_log"));
    t2->setIcon(0, Icons::table());
    auto *db2 = new QTreeWidgetItem(open1, QStringList("KOP_HQ_XXL"));
    db2->setIcon(0, Icons::database());
    new QTreeWidgetItem(db2, QStringList("(展开加载表)"));

    // 未连接 sqlite:灰显
    auto *closed1 = new QTreeWidgetItem(&tree, QStringList("conn-sqlite(未连接)"));
    closed1->setIcon(0, Icons::connection("sqlite", false));
    new QTreeWidgetItem(closed1, QStringList("(展开以连接)"));

    // 未连接 mysql:灰显
    auto *closed2 = new QTreeWidgetItem(&tree, QStringList("conn1(未连接)"));
    closed2->setIcon(0, Icons::connection("mysql", false));
    new QTreeWidgetItem(closed2, QStringList("(展开以连接)"));

    open1->setExpanded(true);
    db1->setExpanded(true);
    tree.setCurrentItem(open1);  // 选中已展开的连接:验证箭头不再消失

    tree.show();
    QString out = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : "tree-preview.png";
    tree.grab().save(out);
    return 0;
}
