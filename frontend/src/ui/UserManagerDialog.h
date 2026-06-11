#ifndef JOOKKIT_USERMANAGERDIALOG_H
#define JOOKKIT_USERMANAGERDIALOG_H

#include <QDialog>
#include <QString>

class QTableWidget;
class BackendClient;

// MySQL 用户管理:列出用户,新建/删除/改密码,查看权限(SHOW GRANTS)。
class UserManagerDialog : public QDialog {
    Q_OBJECT
public:
    UserManagerDialog(BackendClient *client, const QString &connId, QWidget *parent = nullptr);

private slots:
    void reload();
    void addUser();
    void dropUser();
    void changePassword();
    void showGrants();

private:
    bool runSql(const QString &sql);
    QString selUser() const;
    QString selHost() const;

    BackendClient *client_;
    QString connId_;
    QTableWidget *table_;
};

#endif
