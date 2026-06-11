#include "ui/Icons.h"

#include <QPixmap>
#include <QPainter>
#include <QPainterPath>

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

QIcon connection(const QString &type) {
    QPixmap pm = base();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QColor c = (type == "mysql") ? kBlue : (type == "sqlite") ? kAmber : kTeal;
    drawCylinder(p, QRectF(7, 5, 18, 22), c);
    // 连接小标:右下角插头点
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawEllipse(QPointF(25, 25), 4, 4);
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
