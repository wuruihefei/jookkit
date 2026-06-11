#ifndef JOOKKIT_SQLEDITOR_H
#define JOOKKIT_SQLEDITOR_H

#include <QPlainTextEdit>

class SqlHighlighter;

// 带行号栏和当前行高亮的 SQL 编辑器。
class SqlEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit SqlEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth() const;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    QWidget *lineNumberArea_;
    SqlHighlighter *highlighter_;
};

#endif
