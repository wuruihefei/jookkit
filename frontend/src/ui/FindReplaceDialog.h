#ifndef JOOKKIT_FINDREPLACEDIALOG_H
#define JOOKKIT_FINDREPLACEDIALOG_H

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;
class QCheckBox;

// 在指定编辑器内查找/替换。
class FindReplaceDialog : public QDialog {
    Q_OBJECT
public:
    FindReplaceDialog(QPlainTextEdit *editor, QWidget *parent = nullptr);

private slots:
    void findNext();
    void replaceOne();
    void replaceAll();

private:
    QPlainTextEdit *editor_;
    QLineEdit *find_;
    QLineEdit *replace_;
    QCheckBox *caseChk_;
};

#endif
