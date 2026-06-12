// 一次性工具:把 devicon 数据库品牌 SVG 渲染为 64x64 PNG(树节点图标用)。
// 用法: QT_QPA_PLATFORM=offscreen ./mkdbicons <资源目录>
// 依赖 Qt SVG 图像格式插件(libqsvg)。
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QDir>

// 按比例渲染 SVG 并在 64x64 透明画布上居中
static bool renderSvg(const QString &svgPath, const QString &outPath) {
    const int s = 64;
    QImageReader reader(svgPath);
    if (!reader.canRead()) return false;
    QSize sz = reader.size();
    sz.scale(s, s, Qt::KeepAspectRatio);
    reader.setScaledSize(sz);
    QImage src = reader.read();
    if (src.isNull()) return false;

    QImage canvas(s, s, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    QPainter p(&canvas);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(QPointF((s - src.width()) / 2.0, (s - src.height()) / 2.0), src);
    p.end();
    return canvas.save(outPath);
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QDir dir((argc > 1) ? QString::fromLocal8Bit(argv[1]) : ".");
    if (!renderSvg(dir.filePath("mysql-original.svg"), dir.filePath("db-mysql.png"))) return 1;
    if (!renderSvg(dir.filePath("sqlite-original.svg"), dir.filePath("db-sqlite.png"))) return 2;
    return 0;
}
