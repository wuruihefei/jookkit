#ifndef JOOKKIT_IMPORTDIALOG_H
#define JOOKKIT_IMPORTDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include "import/ParseResult.h"

class BackendClient;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QTableWidget;
class QLabel;
class QPushButton;

// 把 CSV / JSON 文件导入到指定表。内存版:解析→预览→列映射→批量 INSERT。
// 入口:左树表节点右键“导入数据到此表”。
class ImportDialog : public QDialog {
    Q_OBJECT
public:
    ImportDialog(BackendClient *client, const QString &connId, const QString &db,
                 const QString &table, const QString &dbType, QWidget *parent = nullptr);

private slots:
    void browseFile();
    void reparse();          // 读文件+按编码/分隔符重新解析,刷新预览与映射
    void doImport();

private:
    void loadTableColumns();     // DESCRIBE_TABLE 取目标表列
    void rebuildMapping();       // 依据文件表头自动匹配,生成映射下拉
    void refreshPreview();
    bool isJson() const;

    BackendClient *client_;
    QString connId_, db_, table_, dbType_;
    QStringList tableCols_;
    ParseResult parsed_;

    QLineEdit   *pathEdit_      = nullptr;
    QComboBox   *encodingCombo_ = nullptr;
    QComboBox   *sepCombo_      = nullptr;
    QCheckBox   *headerCheck_   = nullptr;
    QLabel      *infoLabel_     = nullptr;   // 检测到的编码/行数
    QTableWidget *preview_      = nullptr;
    QTableWidget *mapping_      = nullptr;   // 行=目标列,列1=文件字段下拉
    QPushButton *importBtn_     = nullptr;
};

#endif
