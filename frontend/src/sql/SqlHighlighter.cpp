#include "sql/SqlHighlighter.h"

SqlHighlighter::SqlHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {

    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor(0, 0, 200));
    keywordFormat.setFontWeight(QFont::Bold);
    const QStringList keywords = {
        "select","from","where","insert","into","values","update","set","delete",
        "create","table","drop","alter","add","column","index","view","join","left",
        "right","inner","outer","on","group","by","order","having","limit","offset",
        "and","or","not","null","is","in","like","between","as","distinct","union",
        "primary","key","foreign","references","unique","default","auto_increment",
        "int","integer","varchar","text","char","date","datetime","timestamp",
        "boolean","float","double","decimal","begin","commit","rollback","case",
        "when","then","else","end","count","sum","avg","min","max"
    };
    for (const QString &kw : keywords) {
        Rule r;
        r.pattern = QRegularExpression(
            "\\b" + kw + "\\b", QRegularExpression::CaseInsensitiveOption);
        r.format = keywordFormat;
        rules_.append(r);
    }

    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor(150, 80, 0));
    rules_.append({QRegularExpression("\\b[0-9]+(\\.[0-9]+)?\\b"), numberFormat});

    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor(0, 140, 0));
    rules_.append({QRegularExpression("'[^']*'"), stringFormat});
    rules_.append({QRegularExpression("\"[^\"]*\""), stringFormat});

    QTextCharFormat lineCommentFormat;
    lineCommentFormat.setForeground(QColor(128, 128, 128));
    lineCommentFormat.setFontItalic(true);
    rules_.append({QRegularExpression("--[^\n]*"), lineCommentFormat});

    blockCommentFormat_ = lineCommentFormat;
    blockStart_ = QRegularExpression("/\\*");
    blockEnd_ = QRegularExpression("\\*/");
}

void SqlHighlighter::highlightBlock(const QString &text) {
    for (const Rule &r : rules_) {
        auto it = r.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), r.format);
        }
    }

    // 跨行块注释
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(blockStart_);
    while (startIndex >= 0) {
        auto m = blockEnd_.match(text, startIndex);
        int endIndex = m.capturedStart();
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + m.capturedLength();
        }
        setFormat(startIndex, commentLength, blockCommentFormat_);
        startIndex = text.indexOf(blockStart_, startIndex + commentLength);
    }
}
