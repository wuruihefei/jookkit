#include "ui/SqlEditor.h"
#include "sql/SqlHighlighter.h"

#include <QPainter>
#include <QTextBlock>
#include <QCompleter>
#include <QStringListModel>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QSettings>

namespace {
class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(SqlEditor *editor) : QWidget(editor), editor_(editor) {}
    QSize sizeHint() const override { return QSize(editor_->lineNumberAreaWidth(), 0); }
protected:
    void paintEvent(QPaintEvent *event) override { editor_->lineNumberAreaPaintEvent(event); }
private:
    SqlEditor *editor_;
};

const QStringList kKeywords = {
    "SELECT","FROM","WHERE","INSERT","INTO","VALUES","UPDATE","SET","DELETE",
    "CREATE","TABLE","DROP","ALTER","JOIN","LEFT","RIGHT","INNER","OUTER","ON",
    "GROUP BY","ORDER BY","HAVING","LIMIT","OFFSET","AND","OR","NOT","NULL",
    "IS","IN","LIKE","BETWEEN","AS","DISTINCT","UNION","PRIMARY","KEY","INDEX",
    "COUNT","SUM","AVG","MIN","MAX"
};
}

SqlEditor::SqlEditor(QWidget *parent)
    : QPlainTextEdit(parent),
      lineNumberArea_(new LineNumberArea(this)),
      highlighter_(new SqlHighlighter(document())),
      completer_(new QCompleter(this)) {

    QFont f("Monospace");
    f.setStyleHint(QFont::TypeWriter);
    f.setPointSize(QSettings().value("editor/fontSize", 10).toInt());
    setFont(f);

    completer_->setWidget(this);
    completer_->setCompletionMode(QCompleter::PopupCompletion);
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    completer_->setModel(new QStringListModel(kKeywords, completer_));
    connect(completer_, QOverload<const QString &>::of(&QCompleter::activated),
            this, &SqlEditor::insertCompletion);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &SqlEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &SqlEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &SqlEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void SqlEditor::setCompletionWords(const QStringList &words) {
    QStringList all = kKeywords + words;
    all.removeDuplicates();
    completer_->setModel(new QStringListModel(all, completer_));
}

// 取光标左侧的标识符前缀(含字母/数字/下划线),避免被 '_' 截断。
QString SqlEditor::textUnderCursor() const {
    const QTextCursor tc = textCursor();
    const int pos = tc.positionInBlock();
    const QString line = tc.block().text();
    int start = pos;
    auto isIdent = [](QChar c) { return c.isLetterOrNumber() || c == QLatin1Char('_'); };
    while (start > 0 && isIdent(line.at(start - 1))) --start;
    return line.mid(start, pos - start);
}

void SqlEditor::insertCompletion(const QString &completion) {
    QTextCursor tc = textCursor();
    const int n = completer_->completionPrefix().length();
    tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, n);  // 选中已输入的前缀
    tc.removeSelectedText();
    tc.insertText(completion);                                      // 整段替换
    setTextCursor(tc);
}

void SqlEditor::keyPressEvent(QKeyEvent *e) {
    if (completer_->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return;
        default:
            break;
        }
    }

    const bool ctrlSpace = (e->key() == Qt::Key_Space) && (e->modifiers() & Qt::ControlModifier);
    if (!ctrlSpace)
        QPlainTextEdit::keyPressEvent(e);

    const QString prefix = textUnderCursor();
    if (!ctrlSpace && (e->text().isEmpty() || prefix.length() < 2)) {
        completer_->popup()->hide();
        return;
    }
    if (prefix != completer_->completionPrefix()) {
        completer_->setCompletionPrefix(prefix);
        completer_->popup()->setCurrentIndex(completer_->completionModel()->index(0, 0));
    }
    QRect cr = cursorRect();
    cr.setWidth(completer_->popup()->sizeHintForColumn(0)
                + completer_->popup()->verticalScrollBar()->sizeHint().width());
    completer_->complete(cr);
}

void SqlEditor::toggleComment() {
    QTextCursor cur = textCursor();
    int startBlock = document()->findBlock(cur.selectionStart()).blockNumber();
    int endBlock = document()->findBlock(cur.selectionEnd()).blockNumber();

    bool allCommented = true;
    for (int i = startBlock; i <= endBlock; ++i) {
        QTextBlock b = document()->findBlockByNumber(i);
        if (!b.text().trimmed().startsWith("--")) { allCommented = false; break; }
    }

    QTextCursor edit(document());
    edit.beginEditBlock();
    for (int i = startBlock; i <= endBlock; ++i) {
        QTextBlock b = document()->findBlockByNumber(i);
        if (!b.isValid()) continue;
        if (allCommented) {
            const QString t = b.text();
            int idx = t.indexOf("--");
            if (idx >= 0) {
                int len = (idx + 2 < t.size() && t.at(idx + 2) == ' ') ? 3 : 2;
                edit.setPosition(b.position() + idx);
                edit.setPosition(b.position() + idx + len, QTextCursor::KeepAnchor);
                edit.removeSelectedText();
            }
        } else {
            edit.setPosition(b.position());
            edit.insertText("-- ");
        }
    }
    edit.endEditBlock();
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
        selection.format.setBackground(QColor(236, 253, 245));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
}

void SqlEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), QColor(241, 245, 249));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(148, 163, 184));
            painter.drawText(0, top, lineNumberArea_->width() - 4,
                             fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
