// 一次性工具:渲染 QSS 用的小图标 PNG(树 ::branch 箭头、下拉框 chevron)。
// 用法: QT_QPA_PLATFORM=offscreen ./mkbranch <输出目录>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QDir>

// closed=右三角,open=下三角;slate 色,16x16 留边距
static QImage render(bool open) {
    const int s = 16;
    QImage img(s, s, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#64748b"));
    QPainterPath tri;
    if (open) {
        tri.moveTo(4, 6);  tri.lineTo(12, 6);  tri.lineTo(8, 11);
    } else {
        tri.moveTo(6, 4);  tri.lineTo(11, 8);  tri.lineTo(6, 12);
    }
    tri.closeSubpath();
    p.drawPath(tri);
    p.end();
    return img;
}

// 下拉框/数字框 chevron(细线 v 形,现代扁平风);up=true 朝上
static QImage renderChevron(bool up = false) {
    const int s = 16;
    QImage img(s, s, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(QColor("#64748b"), 1.8);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    QPainterPath path;
    if (up) {
        path.moveTo(4.5, 9.5); path.lineTo(8, 6); path.lineTo(11.5, 9.5);
    } else {
        path.moveTo(4.5, 6.5); path.lineTo(8, 10); path.lineTo(11.5, 6.5);
    }
    p.drawPath(path);
    p.end();
    return img;
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QString dir = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : ".";
    if (!render(false).save(QDir(dir).filePath("branch-closed.png"))) return 1;
    if (!render(true).save(QDir(dir).filePath("branch-open.png"))) return 1;
    if (!renderChevron().save(QDir(dir).filePath("chevron-down.png"))) return 1;
    if (!renderChevron(true).save(QDir(dir).filePath("chevron-up.png"))) return 1;
    return 0;
}
