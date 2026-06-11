// 一次性工具:用 QPainter 渲染 JookKit 图标到多尺寸并拼成 .ico(PNG 内嵌)。
// 用法: QT_QPA_PLATFORM=offscreen ./mkicon <输出.ico>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QList>
#include <QtEndian>

static QImage render(int s) {
    QImage img(s, s, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    const qreal u = s / 64.0;

    // 青绿圆角底
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#0d9488"));
    p.drawRoundedRect(QRectF(6 * u, 6 * u, 52 * u, 52 * u), 14 * u, 14 * u);

    // 白色数据库圆柱
    QRectF r(20 * u, 16 * u, 24 * u, 30 * u);
    const qreal eh = r.height() * 0.22;
    QRectF topE(r.left(), r.top(), r.width(), eh);
    QRectF botE(r.left(), r.bottom() - eh, r.width(), eh);
    p.setPen(QPen(QColor("#ffffff"), qMax(1.0, 2 * u)));
    p.setBrush(QColor(255, 255, 255, 70));
    QPainterPath body;
    body.moveTo(r.left(), r.top() + eh / 2);
    body.lineTo(r.left(), r.bottom() - eh / 2);
    body.arcTo(botE, 180, 180);
    body.lineTo(r.right(), r.top() + eh / 2);
    body.arcTo(topE, 0, 360);
    p.drawPath(body);
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(topE);
    p.end();
    return img;
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    const QList<int> sizes = {16, 24, 32, 48, 64, 128, 256};

    QList<QByteArray> pngs;
    for (int s : sizes) {
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        render(s).save(&buf, "PNG");
        pngs << ba;
    }

    QByteArray out;
    auto u16 = [&](quint16 v) { char b[2]; qToLittleEndian(v, b); out.append(b, 2); };
    auto u32 = [&](quint32 v) { char b[4]; qToLittleEndian(v, b); out.append(b, 4); };

    u16(0); u16(1); u16(quint16(sizes.size()));          // ICONDIR
    quint32 offset = 6 + 16u * sizes.size();
    for (int i = 0; i < sizes.size(); ++i) {
        int s = sizes[i];
        out.append(char(s >= 256 ? 0 : s));              // width
        out.append(char(s >= 256 ? 0 : s));              // height
        out.append(char(0)); out.append(char(0));        // colorCount, reserved
        u16(1); u16(32);                                 // planes, bitCount
        u32(quint32(pngs[i].size()));
        u32(offset);
        offset += pngs[i].size();
    }
    for (const auto &png : pngs) out.append(png);

    QString path = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : "jookkit.ico";
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return 1;
    f.write(out);
    f.close();
    return 0;
}
