#ifndef JOOKKIT_CONTENTWIDGET_H
#define JOOKKIT_CONTENTWIDGET_H

#include <QWidget>
#include <QString>
#include <QList>
#include "backend/ConnData.h"

class QTabWidget;
class QWidget;
class ObjectTree;
class InformationPane;
class BackendClient;
class QueryForm;

// 中心枢纽:左(对象工具栏 + 对象树) + 右(标签页 + 信息窗格)。
class ContentWidget : public QWidget {
    Q_OBJECT
public:
    explicit ContentWidget(BackendClient *client, QWidget *parent = nullptr);

    bool addConnection(const ConnData &c);
    void addSavedConnection(const ConnData &c);
    QList<ConnData> allConnections() const;
    ObjectTree *tree() const { return tree_; }
    QueryForm *currentQueryForm() const;

    bool isSidebarVisible() const;
    bool isInfoVisible() const;
    void setSidebarVisible(bool on);
    void setInfoVisible(bool on);

public slots:
    void newQuery();
    void viewCurrentData();
    void viewCurrentStructure();
    void openTableData(const QString &connId, const QString &db, const QString &table);
    void openTableStructure(const QString &connId, const QString &db, const QString &table);
    void closeCurrentTab();
    void closeOtherTabs();
    void closeAllTabs();
    void previousTab();
    void nextTab();
    void toggleSidebar();
    void toggleInfo();

private slots:
    void updateInfo();

private:
    void addTab(QWidget *w, const QString &title);

    BackendClient *client_;
    QWidget *leftPanel_;
    ObjectTree *tree_;
    QTabWidget *tabs_;
    InformationPane *info_;
};

#endif
