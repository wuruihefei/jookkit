#include "ui/SqlEditor.h"
#include "sql/SqlHighlighter.h"

#include <QPainter>
#include <QTextBlock>

namespace {
// 行号栏:把绘制委托回 SqlEditor。
class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(SqlEditor *editor) : QWidget(editor), editor_(editor) {}
    QSize sizeHint() const override { return QSize(editor_->lineNumberAreaWidth(), 0); }
protected:
    void paintEvent(QPaintEvent *event) override { editor_->lineNumberAreaPaintEvent(event); }
private:
    SqlEditor *editor_;
};
}

SqlEditor::SqlEditor(QWidget *parent)
    : QPlainTextEdit(parent),
      lineNumberArea_(new LineNumberArea(this)),
      highlighter_(new SqlHighlighter(document())) {

    QFont f("Monospace");
    f.setStyleHint(QFont::TypeWriter);
    setFont(f);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &SqlEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &SqlEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &SqlEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int SqlEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void SqlEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void SqlEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea_->scroll(0, dy);
    else
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void SqlEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void SqlEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(232, 242, 254));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
}

void SqlEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), QColor(240, 240, 240));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(120, 120, 120));
            painter.drawText(0, top, lineNumberArea_->width() - 4,
                             fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
