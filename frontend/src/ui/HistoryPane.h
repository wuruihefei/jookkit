#ifndef JOOKKIT_HISTORYPANE_H
#define JOOKKIT_HISTORYPANE_H

#include <QWidget>
#include "store/HistoryStore.h"

class QLineEdit;
class QListWidget;

// SQL 执行历史面板：搜索框 + 列表；双击触发 sqlChosen。
class HistoryPane : public QWidget {
    Q_OBJECT
public:
    explicit HistoryPane(QWidget *parent = nullptr);
    void refresh();

signals:
    void sqlChosen(const QString &sql);

private slots:
    void applyFilter(const QString &text);
    void onItemActivated();
    void showMenu(const QPoint &pos);

private:
    void rebuild();

    QLineEdit *search_;
    QListWidget *list_;
    QList<HistoryEntry> entries_;
    QString filter_;
};

#endif
