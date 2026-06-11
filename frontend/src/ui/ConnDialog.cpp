#include "ui/ConnDialog.h"
#include "backend/BackendClient.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QStackedWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>

ConnDialog::ConnDialog(BackendClient *client, QWidget *parent)
    : QDialog(parent), client_(client) {
    setWindowTitle(tr("新建连接"));

    nameEdit_ = new QLineEdit("conn1");
    typeCombo_ = new QComboBox;
    typeCombo_->addItems({"sqlite", "mysql"});

    // sqlite 页
    auto *sqlitePage = new QWidget;
    fileEdit_ = new QLineEdit;
    auto *browseBtn = new QPushButton(tr("浏览..."));
    auto *fileRow = new QHBoxLayout;
    fileRow->addWidget(fileEdit_);
    fileRow->addWidget(browseBtn);
    auto *sqliteForm = new QFormLayout(sqlitePage);
    sqliteForm->addRow(tr("文件:"), fileRow);

    // mysql 页
    auto *mysqlPage = new QWidget;
    hostEdit_ = new QLineEdit("127.0.0.1");
    portSpin_ = new QSpinBox; portSpin_->setRange(1, 65535); portSpin_->setValue(3306);
    userEdit_ = new QLineEdit("root");
    pwdEdit_ = new QLineEdit; pwdEdit_->setEchoMode(QLineEdit::Password);
    dbEdit_ = new QLineEdit;
    auto *mysqlForm = new QFormLayout(mysqlPage);
    mysqlForm->addRow(tr("主机:"), hostEdit_);
    mysqlForm->addRow(tr("端口:"), portSpin_);
    mysqlForm->addRow(tr("用户:"), userEdit_);
    mysqlForm->addRow(tr("密码:"), pwdEdit_);
    mysqlForm->addRow(tr("数据库:"), dbEdit_);

    stack_ = new QStackedWidget;
    stack_->addWidget(sqlitePage);
    stack_->addWidget(mysqlPage);

    auto *testBtn = new QPushButton(tr("测试连接"));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    auto *top = new QFormLayout;
    top->addRow(tr("名称:"), nameEdit_);
    top->addRow(tr("类型:"), typeCombo_);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(stack_);
    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(testBtn);
    btnRow->addStretch();
    btnRow->addWidget(buttons);
    layout->addLayout(btnRow);

    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnDialog::onTypeChanged);
    connect(browseBtn, &QPushButton::clicked, this, &ConnDialog::browseFile);
    connect(testBtn, &QPushButton::clicked, this, &ConnDialog::testConnection);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ConnDialog::onTypeChanged(int index) {
    stack_->setCurrentIndex(index);
}

void ConnDialog::browseFile() {
    QString f = QFileDialog::getOpenFileName(this, tr("选择 SQLite 文件"));
    if (!f.isEmpty()) fileEdit_->setText(f);
}

ConnData ConnDialog::build() const {
    ConnData c;
    c.connId = nameEdit_->text().trimmed();
    c.type = typeCombo_->currentText();
    if (c.type == "sqlite") {
        c.file = fileEdit_->text().trimmed();
    } else {
        c.host = hostEdit_->text().trimmed();
        c.port = portSpin_->value();
        c.user = userEdit_->text();
        c.password = pwdEdit_->text();
        c.database = dbEdit_->text().trimmed();
    }
    return c;
}

ConnData ConnDialog::connData() const {
    return build();
}

void ConnDialog::testConnection() {
    if (!client_) return;
    auto r = client_->call(build().toTestRequest(), 10000);
    if (r.ok)
        QMessageBox::information(this, tr("测试连接"), tr("连接成功"));
    else
        QMessageBox::warning(this, tr("测试连接"),
                             tr("连接失败: %1").arg(r.errorMessage));
}
