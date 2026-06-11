#ifndef JOOKKIT_SQLEDITOR_H
#define JOOKKIT_SQLEDITOR_H

#include <QPlainTextEdit>
#include <QStringList>

class SqlHighlighter;
class QCompleter;

// 带行号栏、当前行高亮、语法高亮和自动补全的 SQL 编辑器。
class SqlEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit SqlEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth() const;
    void toggleComment();
    void setCompletionWords(const QStringList &words);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();
    void insertCompletion(const QString &completion);

private:
    QString textUnderCursor() const;

    QWidget *lineNumberArea_;
    SqlHighlighter *highlighter_;
    QCompleter *completer_;
};

#endif
