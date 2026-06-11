#include "ui/UserManagerDialog.h"
#include "backend/BackendClient.h"
#include "backend/FuncId.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QJsonObject>
#include <QJsonArray>
#include <QPlainTextEdit>

UserManagerDialog::UserManagerDialog(BackendClient *client, const QString &connId, QWidget *parent)
    : QDialog(parent), client_(client), connId_(connId) {
    setWindowTitle(tr("用户管理"));
    resize(460, 380);

    table_ = new QTableWidget;
    table_->setColumnCount(2);
    table_->setHorizontalHeaderLabels({tr("用户"), tr("主机")});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *addBtn = new QPushButton(tr("新建用户"));
    auto *delBtn = new QPushButton(tr("删除用户"));
    auto *pwdBtn = new QPushButton(tr("改密码"));
    auto *grantBtn = new QPushButton(tr("查看权限"));
    auto *refreshBtn = new QPushButton(tr("刷新"));
    connect(addBtn, &QPushButton::clicked, this, &UserManagerDialog::addUser);
    connect(delBtn, &QPushButton::clicked, this, &UserManagerDialog::dropUser);
    connect(pwdBtn, &QPushButton::clicked, this, &UserManagerDialog::changePassword);
    connect(grantBtn, &QPushButton::clicked, this, &UserManagerDialog::showGrants);
    connect(refreshBtn, &QPushButton::clicked, this, &UserManagerDialog::reload);

    auto *btns = new QHBoxLayout;
    btns->addWidget(addBtn);
    btns->addWidget(delBtn);
    btns->addWidget(pwdBtn);
    btns->addWidget(grantBtn);
    btns->addStretch();
    btns->addWidget(refreshBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(table_, 1);
    layout->addLayout(btns);

    reload();
}

bool UserManagerDialog::runSql(const QString &sql) {
    QJsonObject req;
    req.insert("funcId", FuncId::EXEC_SQL);
    req.insert("connId", connId_);
    req.insert("sql", sql);
    auto r = client_->call(req);
    if (!r.ok) {
        QMessageBox::warning(this, tr("操作失败"), r.errorMessage);
        return false;
    }
    return true;
}

QString UserManagerDialog::selUser() const {
    int row = table_->currentRow();
    return (row >= 0 && table_->item(row, 0)) ? table_->item(row, 0)->text() : QString();
}
QString UserManagerDialog::selHost() const {
    int row = table_->currentRow();
    return (row >= 0 && table_->item(row, 1)) ? table_->item(row, 1)->text() : QString();
}

void UserManagerDialog::reload() {
    QJsonObject req;
    req.insert("funcId", FuncId::LIST_USERS);
    req.insert("connId", connId_);
    auto r = client_->call(req);
    table_->setRowCount(0);
    if (!r.ok) return;
    if (!r.data.value("supported").toBool()) {
        QMessageBox::information(this, tr("用户管理"), tr("当前连接不支持用户管理(仅 MySQL)"));
        return;
    }
    QJsonArray users = r.data.value("users").toArray();
    table_->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        QJsonObject u = users.at(i).toObject();
        table_->setItem(i, 0, new QTableWidgetItem(u.value("user").toString()));
        table_->setItem(i, 1, new QTableWidgetItem(u.value("host").toString()));
    }
}

void UserManagerDialog::addUser() {
    bool ok = false;
    QString user = QInputDialog::getText(this, tr("新建用户"), tr("用户名:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (!ok || user.trimmed().isEmpty()) return;
    QString host = QInputDialog::getText(this, tr("新建用户"), tr("主机:"),
                                         QLineEdit::Normal, "%", &ok);
    if (!ok) return;
    QString pwd = QInputDialog::getText(this, tr("新建用户"), tr("密码:"),
                                        QLineEdit::Password, QString(), &ok);
    if (!ok) return;
    if (runSql(QString("CREATE USER '%1'@'%2' IDENTIFIED BY '%3'")
             .arg(user.trimmed(), host.trimmed(), pwd)))
        reload();
}

void UserManagerDialog::dropUser() {
    if (selUser().isEmpty()) { QMessageBox::information(this, tr("提示"), tr("请先选中一个用户")); return; }
    if (QMessageBox::question(this, tr("删除用户"),
            tr("确定删除 '%1'@'%2' ?").arg(selUser(), selHost())) != QMessageBox::Yes)
        return;
    if (runSql(QString("DROP USER '%1'@'%2'").arg(selUser(), selHost())))
        reload();
}

void UserManagerDialog::changePassword() {
    if (selUser().isEmpty()) { QMessageBox::information(this, tr("提示"), tr("请先选中一个用户")); return; }
    bool ok = false;
    QString pwd = QInputDialog::getText(this, tr("改密码"),
            tr("'%1'@'%2' 的新密码:").arg(selUser(), selHost()),
            QLineEdit::Password, QString(), &ok);
    if (!ok) return;
    if (runSql(QString("ALTER USER '%1'@'%2' IDENTIFIED BY '%3'").arg(selUser(), selHost(), pwd)))
        QMessageBox::information(this, tr("改密码"), tr("已修改"));
}

void UserManagerDialog::showGrants() {
    if (selUser().isEmpty()) { QMessageBox::information(this, tr("提示"), tr("请先选中一个用户")); return; }
    QJsonObject req;
    req.insert("funcId", FuncId::EXEC_SQL);
    req.insert("connId", connId_);
    req.insert("sql", QString("SHOW GRANTS FOR '%1'@'%2'").arg(selUser(), selHost()));
    auto r = client_->call(req);
    if (!r.ok) { QMessageBox::warning(this, tr("查看权限"), r.errorMessage); return; }
    QString text;
    for (const auto &row : r.data.value("rows").toArray())
        text += row.toArray().at(0).toString() + "\n";

    QDialog dlg(this);
    dlg.setWindowTitle(tr("权限: '%1'@'%2'").arg(selUser(), selHost()));
    dlg.resize(560, 300);
    auto *te = new QPlainTextEdit(text);
    te->setReadOnly(true);
    auto *l = new QVBoxLayout(&dlg);
    l->addWidget(te);
    dlg.exec();
}
