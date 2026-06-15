#include "import/CsvReader.h"
#include <QTextCodec>

namespace {

bool hasUtf8Bom(const QByteArray &b) {
    return b.size() >= 3 &&
           (unsigned char)b.at(0) == 0xEF &&
           (unsigned char)b.at(1) == 0xBB &&
           (unsigned char)b.at(2) == 0xBF;
}

// 解码字节流;返回 QString,并通过 outName 返回实际编码名。
// 优先级:forcedCodec > UTF-8 BOM > 严格 UTF-8 > GBK 回退。
QString decodeBytes(const QByteArray &bytes, const QString &forced, QString &outName) {
    if (!forced.isEmpty()) {
        QTextCodec *codec = QTextCodec::codecForName(forced.toUtf8());
        if (codec) {
            outName = forced;
            if (forced.compare("UTF-8", Qt::CaseInsensitive) == 0 && hasUtf8Bom(bytes))
                return codec->toUnicode(bytes.mid(3));
            return codec->toUnicode(bytes);
        }
    }
    if (hasUtf8Bom(bytes)) {
        outName = QStringLiteral("UTF-8");
        return QString::fromUtf8(bytes.constData() + 3, bytes.size() - 3);
    }
    // 严格试 UTF-8:无非法字节即认定 UTF-8
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec::ConverterState state;
    QString decoded = utf8->toUnicode(bytes.constData(), bytes.size(), &state);
    if (state.invalidChars == 0) {
        outName = QStringLiteral("UTF-8");
        return decoded;
    }
    // 回退 GBK
    if (QTextCodec *gbk = QTextCodec::codecForName("GBK")) {
        outName = QStringLiteral("GBK");
        return gbk->toUnicode(bytes);
    }
    outName = QStringLiteral("UTF-8");
    return decoded;
}

} // namespace

namespace CsvReader {

ParseResult read(const QByteArray &bytes, const CsvOptions &opt) {
    ParseResult res;
    QString codecName;
    const QString text = decodeBytes(bytes, opt.forcedCodec, codecName);
    res.detectedCodec = codecName;

    const int n = text.size();
    const QChar sep = opt.sep;

    bool inQuotes = false;
    bool recordOpen = false;
    int  recordStartLine = -1;
    int  lineNo = 1;

    QStringList fields;
    QString field;          // null = 未加引号的空字段
    bool fieldQuoted = false;

    auto startRecord = [&]() {
        if (!recordOpen) { recordOpen = true; recordStartLine = lineNo; }
    };
    auto endField = [&]() {
        if (fieldQuoted)
            fields << (field.isNull() ? QString("") : field);  // 引号空 → 空串非 NULL
        else
            fields << field;                                    // 未引号空 → 保持 null(NULL)
        field = QString();
        fieldQuoted = false;
    };
    auto endRecord = [&]() {
        endField();
        res.rows << fields;
        res.sourceLines << recordStartLine;
        fields.clear();
        recordOpen = false;
    };

    int i = 0;
    while (i < n) {
        const QChar c = text.at(i);
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < n && text.at(i + 1) == '"') { field += '"'; i += 2; continue; }
                inQuotes = false; ++i; continue;          // 闭合引号
            }
            if (c == '\n') ++lineNo;                        // 字段内换行仍占物理行
            field += c; ++i; continue;
        }
        if (c == '"') {
            startRecord();
            fieldQuoted = true;
            if (field.isNull()) field = QString("");        // 标记非 null 起始
            inQuotes = true; ++i; continue;
        }
        if (c == sep) {
            startRecord();
            endField(); ++i; continue;
        }
        if (c == '\n') {
            if (recordOpen) endRecord();
            ++lineNo; ++i; continue;
        }
        if (c == '\r') {
            if (recordOpen) endRecord();
            if (i + 1 < n && text.at(i + 1) == '\n') { ++lineNo; i += 2; }
            else { ++lineNo; ++i; }
            continue;
        }
        startRecord();
        field += c; ++i;
    }

    if (inQuotes) {
        res.ok = false;
        res.error = QStringLiteral("未闭合的引号(到文件末尾)");
        return res;
    }
    if (recordOpen) endRecord();

    if (opt.firstRowHeader && !res.rows.isEmpty()) {
        res.headers = res.rows.takeFirst();
        res.sourceLines.removeFirst();
    }
    return res;
}

} // namespace CsvReader
