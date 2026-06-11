#ifndef JOOKKIT_ICONS_H
#define JOOKKIT_ICONS_H

#include <QIcon>

// 程序化绘制的扁平图标(免外部图片资源),统一青绿主色调。
namespace Icons {
    QIcon connection(const QString &type = QString());  // 数据源(按类型着色)
    QIcon database();
    QIcon table();
    QIcon query();
    QIcon run();
    QIcon data();        // 表数据(网格)
    QIcon structure();   // 表结构(列)
    QIcon refresh();
    QIcon save();
    QIcon add();         // 新增(绿色+)
    QIcon remove();      // 删除(红色-)
    QIcon app();         // 应用图标
}

#endif
