#ifndef JOOKKIT_OPTIONSDIALOG_H
#define JOOKKIT_OPTIONSDIALOG_H

#include <QDialog>

class QComboBox;
class QSpinBox;

// 偏好设置:默认每页条数、编辑器字体大小、历史保留条数/天数(存 QSettings)。
class OptionsDialog : public QDialog {
    Q_OBJECT
public:
    explicit OptionsDialog(QWidget *parent = nullptr);

private slots:
    void apply();

private:
    QComboBox *pageSize_;
    QSpinBox *fontSize_;
    QSpinBox *histMaxCount_ = nullptr;
    QSpinBox *histMaxDays_  = nullptr;
};

#endif
