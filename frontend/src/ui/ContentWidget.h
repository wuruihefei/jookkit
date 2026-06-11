#ifndef JOOKKIT_CONTENTWIDGET_H
#define JOOKKIT_CONTENTWIDGET_H

#include <QWidget>
#include <QString>
#include "backend/ConnData.h"

class QTabWidget;
class ObjectTree;
class BackendClient;
class QueryForm;

// 中心枢纽:左对象树 + 右标签页。
class ContentWidget : public QWidget {
    Q_OBJECT
public:
    explicit ContentWidget(BackendClient *client, QWidget *parent = nullptr);

    bool addConnection(const ConnData &c);
    ObjectTree *tree() const { return tree_; }
    QueryForm *currentQueryForm() const;

public slots:
    void newQuery();
    void viewCurrentData();
    void viewCurrentStructure();
    void openTableData(const QString &connId, const QString &db, const QString &table);
    void openTableStructure(const QString &connId, const QString &db, const QString &table);
    // 标签 / 侧栏
    void closeCurrentTab();
    void closeOtherTabs();
    void closeAllTabs();
    void previousTab();
    void nextTab();
    void toggleSidebar();

private:
    void addTab(QWidget *w, const QString &title);

    BackendClient *client_;
    ObjectTree *tree_;
    QTabWidget *tabs_;
};

#endif
