#include "ui/Icons.h"

#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {
const QColor kTeal("#0d9488");
const QColor kBlue("#2563eb");
const QColor kAmber("#d97706");
const QColor kSlate("#475569");

QPixmap base(int s = 32) {
    QPixmap pm(s, s);
    pm.fill(Qt::transparent);
    return pm;
}

QIcon fromPm(const QPixmap &pm) { return QIcon(pm); }

// 数据库圆柱
void drawCylinder(QPainter &p, const QRectF &r, const QColor &c) {
    p.setPen(QPen(c, 2));
    p.setBrush(c.lighter(180));
    qreal eh = r.height() * 0.22;
    QRectF topE(r.left(), r.top(), r.width(), eh);
    QRectF botE(r.left(), r.bottom() - eh, r.width(), eh);
    QPainterPath body;
    body.moveTo(r.left(), r.top() + eh / 2);
    body.lineTo(r.left(), r.bottom() - eh / 2);
    body.arcTo(botE, 180, 180);
    body.lineTo(r.right(), r.top() + eh / 2);
    body.arcTo(topE, 0, 360);
    p.drawPath(body);
    p.setBrush(c.lighter(140));
    p.drawEllipse(topE);
}
}

namespace Icons {

QIcon connection(const QString &type, bool open) {
    // 已知类型用品牌 logo(devicon, MIT):MySQL 海豚 / SQLite 羽毛
    const QString res = (type == "mysql") ? ":/resources/db-mysql.png"
                      : (type == "sqlite") ? ":/resources/db-sqlite.png" : QString();
    if (!res.isEmpty()) {
        QImage img(res);
        if (!img.isNull()) {
            img = img.convertToFormat(QImage::Format_ARGB32);
            if (!open) {
                // 未连接:去饱和 + 降透明,整体灰显
                for (int y = 0; y < img.height(); ++y) {
                    auto *line = reinterpret_cast<QRgb *>(img.scanLine(y));
                    for (int x = 0; x < img.width(); ++x) {
                        const QRgb px = line[x];
                        const int g = qGray(px);
                        line[x] = qRgba(g, g, g, qAlpha(px) * 55 / 100);
                    }
                }
            }
            QPixmap pm = QPixmap::fromImage(img);  // 64x64
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing);
            if (open) {  // 右下角绿色状态点(白描边)
                p.setPen(QPen(QColor("#ffffff"), 3));
                p.setBrush(QColor("#16a34a"));
                p.drawEllipse(QPointF(53, 53), 9, 9);
            }
            p.end();
            return fromPm(pm);
        }
    }
    // 未知类型:程序化绘制的通用服务器机箱
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QColor c = !open ? QColor("#94a3b8") : kTeal;
    // 服务器机箱:上下两层
    p.setPen(QPen(c, 2));
    p.setBrush(open ? c.lighter(185) : QColor("#e2e8f0"));
    p.drawRoundedRect(QRectF(4, 6, 22, 9), 2, 2);
    p.drawRoundedRect(QRectF(4, 17, 22, 9), 2, 2);
    // 每层:指示灯 + 通风槽
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawEllipse(QPointF(8.5, 10.5), 1.8, 1.8);
    p.drawEllipse(QPointF(8.5, 21.5), 1.8, 1.8);
    p.setBrush(c.lighter(130));
    p.drawRoundedRect(QRectF(13, 9.4, 10, 2.2), 1.1, 1.1);
    p.drawRoundedRect(QRectF(13, 20.4, 10, 2.2), 1.1, 1.1);
    // 右下角状态点:已连接=绿色实心(白描边),未连接=灰色空心
    if (open) {
        p.setPen(QPen(QColor("#ffffff"), 1.6));
        p.setBrush(QColor("#16a34a"));
        p.drawEllipse(QPointF(25.5, 25.5), 4.4, 4.4);
    } else {
        p.setPen(QPen(QColor("#94a3b8"), 1.8));
        p.setBrush(QColor("#f8fafc"));
        p.drawEllipse(QPointF(25.5, 25.5), 4.0, 4.0);
    }
    return fromPm(pm);
}

QIcon database() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    drawCylinder(p, QRectF(7, 5, 18, 22), kTeal);
    return fromPm(pm);
}

QIcon table() { return data(); }

QIcon data() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r(6, 7, 20, 18);
    p.setPen(QPen(kTeal, 2));
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(r, 3, 3);
    p.setBrush(kTeal.lighter(170));
    p.drawRect(QRectF(r.left(), r.top(), r.width(), 5)); // 表头行
    p.setPen(QPen(kTeal.lighter(130), 1));
    p.drawLine(r.left(), r.top() + 5, r.right(), r.top() + 5);
    p.drawLine(r.left(), r.top() + 11, r.right(), r.top() + 11);
    p.drawLine(r.center().x(), r.top() + 5, r.center().x(), r.bottom());
    return fromPm(pm);
}

QIcon structure() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    // 三行“字段”条
    for (int i = 0; i < 3; ++i) {
        p.setBrush(kTeal);
        p.drawEllipse(QPointF(8, 9 + i * 7), 2.2, 2.2);
        p.setBrush(kSlate.lighter(160));
        p.drawRoundedRect(QRectF(13, 7.5 + i * 7, 13, 3), 1.5, 1.5);
    }
    return fromPm(pm);
}

QIcon query() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r(8, 5, 16, 22);
    p.setPen(QPen(kSlate, 2));
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(r, 2, 2);
    p.setPen(QPen(kTeal, 1.6));
    for (int i = 0; i < 4; ++i)
        p.drawLine(r.left() + 3, r.top() + 5 + i * 4, r.right() - 3, r.top() + 5 + i * 4);
    return fromPm(pm);
}

QIcon run() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#16a34a")); // 运行=绿色三角
    QPainterPath tri;
    tri.moveTo(11, 7);
    tri.lineTo(25, 16);
    tri.lineTo(11, 25);
    tri.closeSubpath();
    p.drawPath(tri);
    return fromPm(pm);
}

QIcon refresh() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(kTeal, 2.4));
    p.setBrush(Qt::NoBrush);
    QRectF r(9, 9, 14, 14);
    p.drawArc(r, 60 * 16, 250 * 16);
    p.setPen(Qt::NoPen);
    p.setBrush(kTeal);
    QPainterPath ar;
    ar.moveTo(20, 7); ar.lineTo(24, 11); ar.lineTo(18.5, 12); ar.closeSubpath();
    p.drawPath(ar);
    return fromPm(pm);
}

QIcon save() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(kSlate, 2));
    p.setBrush(kTeal.lighter(170));
    p.drawRoundedRect(QRectF(7, 7, 18, 18), 2, 2);
    p.setBrush(QColor("#ffffff"));
    p.drawRect(QRectF(11, 7, 10, 7)); // 标签
    p.setBrush(kSlate.lighter(150));
    p.drawRect(QRectF(11, 17, 10, 7)); // 主体
    return fromPm(pm);
}

QIcon add() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#16a34a"));
    p.drawEllipse(QPointF(16, 16), 11, 11);
    p.setPen(QPen(QColor("#ffffff"), 2.6));
    p.drawLine(16, 10, 16, 22);
    p.drawLine(10, 16, 22, 16);
    return fromPm(pm);
}

QIcon remove() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#dc2626"));
    p.drawEllipse(QPointF(16, 16), 11, 11);
    p.setPen(QPen(QColor("#ffffff"), 2.6));
    p.drawLine(10, 16, 22, 16);
    return fromPm(pm);
}

// 电源符号:缺口圆弧 + 顶部竖线
static QIcon powerIcon(const QColor &c) {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(c, 2.8);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    QRectF r(7, 8, 18, 18);
    p.drawArc(r, 120 * 16, 300 * 16);  // 顶部留缺口
    p.drawLine(QPointF(16, 4), QPointF(16, 14));
    return fromPm(pm);
}

QIcon plugOn()  { return powerIcon(QColor("#16a34a")); }
QIcon plugOff() { return powerIcon(QColor("#94a3b8")); }

QIcon edit() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    // 铅笔:笔杆(45°) + 笔尖
    p.save();
    p.translate(16, 16);
    p.rotate(45);
    p.setBrush(kAmber);
    p.drawRoundedRect(QRectF(-3.2, -14, 6.4, 19), 2, 2);
    p.setBrush(kSlate);
    QPainterPath tip;
    tip.moveTo(-3.2, 5);  tip.lineTo(3.2, 5);  tip.lineTo(0, 12);
    tip.closeSubpath();
    p.drawPath(tip);
    p.restore();
    return fromPm(pm);
}

QIcon copy() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(kTeal, 2));
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(QRectF(10, 5, 15, 17), 2, 2);   // 后层
    p.setBrush(kTeal.lighter(180));
    p.drawRoundedRect(QRectF(6, 10, 15, 17), 2, 2);   // 前层
    return fromPm(pm);
}

QIcon trash() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    const QColor red("#dc2626");
    p.setPen(QPen(red, 2));
    p.setBrush(red.lighter(170));
    // 桶身(梯形圆角)
    p.drawRoundedRect(QRectF(9, 11, 14, 16), 2.5, 2.5);
    // 盖与提手
    p.setBrush(red);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(6.5, 7.5, 19, 3), 1.5, 1.5);
    p.drawRoundedRect(QRectF(12.5, 4.5, 7, 3), 1.5, 1.5);
    // 桶身竖纹
    p.setPen(QPen(red, 1.6));
    p.drawLine(QPointF(13.5, 14.5), QPointF(13.5, 23.5));
    p.drawLine(QPointF(18.5, 14.5), QPointF(18.5, 23.5));
    return fromPm(pm);
}

QIcon star() {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#d97706"), 1.6));
    p.setBrush(QColor("#fbbf24"));
    QPainterPath path;
    const QPointF c(16, 17);
    for (int i = 0; i < 10; ++i) {
        const qreal r = (i % 2 == 0) ? 12.0 : 5.2;
        const qreal a = -M_PI / 2 + i * M_PI / 5;
        const QPointF pt(c.x() + r * qCos(a), c.y() + r * qSin(a));
        if (i == 0) path.moveTo(pt); else path.lineTo(pt);
    }
    path.closeSubpath();
    p.drawPath(path);
    return fromPm(pm);
}

QIcon app() {
    QPixmap pm = base(64);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(kTeal);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(6, 6, 52, 52, 14, 14);
    drawCylinder(p, QRectF(20, 16, 24, 30), QColor("#ffffff"));
    return fromPm(pm);
}

}
