#ifndef JOOKKIT_TABLEDATAFORM_H
#define JOOKKIT_TABLEDATAFORM_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QSet>

class QTableWidget;
class QTableWidgetItem;
class QLabel;
class BackendClient;

// 表数据网格:分页加载,就地编辑回写(UPDATE),新增行(INSERT),删除行(DELETE)。
// 编辑/删除依赖主键定位;无主键的表只读并提示。
class TableDataForm : public QWidget {
    Q_OBJECT
public:
    TableDataForm(BackendClient *client, const QString &connId,
                  const QString &db, const QString &table,
                  QWidget *parent = nullptr);

private slots:
    void reload();
    void prevPage();
    void nextPage();
    void changePageSize(int idx);
    void onItemChanged(QTableWidgetItem *item);
    void addRow();
    void saveNewRows();
    void deleteSelectedRow();

private:
    QMap<QString, QString> pkOf(int row) const;
    void updatePageLabel(int rowsThisPage);

    BackendClient *client_;
    QString connId_;
    QString db_;
    QString table_;

    QTableWidget *grid_;
    QLabel *status_;
    QLabel *pageLabel_;

    QStringList columns_;
    QStringList primaryKeys_;
    bool loading_ = false;
    QList<QMap<QString, QString>> rowOriginals_;
    QSet<int> newRows_;

    int page_ = 0;        // 0-based 页码
    int pageSize_ = 200;  // 每页条数
};

#endif
