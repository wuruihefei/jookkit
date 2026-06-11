#ifndef JOOKKIT_SQLHIGHLIGHTER_H
#define JOOKKIT_SQLHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

// 简单的 SQL 语法高亮:关键字、字符串、数字、注释。
class SqlHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit SqlHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule { QRegularExpression pattern; QTextCharFormat format; };
    QVector<Rule> rules_;
    QTextCharFormat blockCommentFormat_;
    QRegularExpression blockStart_;
    QRegularExpression blockEnd_;
};

#endif
