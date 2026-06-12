#ifndef JOOKKIT_CONNDIALOG_H
#define JOOKKIT_CONNDIALOG_H

#include <QDialog>
#include "backend/ConnData.h"

class QLineEdit;
class QSpinBox;
class QComboBox;
class QStackedWidget;
class BackendClient;

// 新建连接对话框:支持 SQLite / MySQL,可测试连接。
class ConnDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConnDialog(BackendClient *client, QWidget *parent = nullptr);
    ConnData connData() const;
    // 回填既有连接配置(编辑模式,标题改为「编辑连接」)
    void setConnData(const ConnData &c);

private slots:
    void onTypeChanged(int index);
    void browseFile();
    void testConnection();

private:
    ConnData build() const;

    BackendClient *client_;
    QLineEdit *nameEdit_;
    QComboBox *typeCombo_;
    QStackedWidget *stack_;
    // sqlite
    QLineEdit *fileEdit_;
    // mysql
    QLineEdit *hostEdit_;
    QSpinBox *portSpin_;
    QLineEdit *userEdit_;
    QLineEdit *pwdEdit_;
    QLineEdit *dbEdit_;
    QLineEdit *paramsEdit_;
};

#endif
