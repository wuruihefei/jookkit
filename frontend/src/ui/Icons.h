#ifndef JOOKKIT_ICONS_H
#define JOOKKIT_ICONS_H

#include <QIcon>

// 程序化绘制的扁平图标(免外部图片资源),统一青绿主色调。
namespace Icons {
    // 连接节点:服务器机箱形(区别于数据库圆柱)。按类型着色;
    // open=true 带绿色状态点,false 整体灰显(未连接)。
    QIcon connection(const QString &type = QString(), bool open = true);
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
    QIcon plugOn();      // 打开连接(绿色电源)
    QIcon plugOff();     // 关闭连接(灰色电源)
    QIcon edit();        // 编辑(铅笔)
    QIcon copy();        // 复制(双层文档)
    QIcon trash();       // 删除(红色垃圾桶)
    QIcon star();        // 收藏(琥珀色五角星)
    QIcon app();         // 应用图标
}

#endif
