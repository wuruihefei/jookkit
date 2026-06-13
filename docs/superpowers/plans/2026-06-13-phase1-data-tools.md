# 阶段一前端三件 实现计划(SQL 历史 / 结果网格增强 / 导出)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 给 JookKit 前端加上 SQL 执行历史、只读结果网格增强(筛选/排序/复制/大值查看)、结果导出(CSV/JSON/SQL INSERT)。

**Architecture:** 纯逻辑模块(`Exporter`、`HistoryStore`)与 UI 解耦、可单测;UI 顺着现有 `QTableWidget` 与 `QDockWidget` 做,不改网格架构;SQL 历史在 `BackendClient::call` 统一出口按 funcId 白名单记录。

**Tech Stack:** C++17 / Qt 5(core/gui/widgets/network)、QtTest、qmake。

设计来源:`docs/superpowers/specs/2026-06-13-phase1-data-tools-design.md`。

---

## 文件结构

新增(前端 `frontend/src/`):

| 文件 | 职责 |
|---|---|
| `export/Exporter.h` / `.cpp` | 纯函数:`headers + rows` → CSV / JSON / INSERT 字节流。不依赖 UI。 |
| `store/HistoryStore.h` / `.cpp` | SQL 历史读写:追加 `history.jsonl`、加载、裁剪(条数/天数)、清空。 |
| `ui/HistoryPane.h` / `.cpp` | 历史面板(`QWidget`,装进 `QDockWidget`):搜索框 + 列表 + 右键。 |
| `ui/GridUtils.h` / `.cpp` | 从 `QTableWidget` 提取 `headers + 可见行 rows`、复制/大值查看辅助。 |

修改:

| 文件 | 改动 |
|---|---|
| `src/backend/BackendClient.h/.cpp` | `call` 内按 funcId 白名单记录历史;新增可注入的「当前 db」上下文。 |
| `src/ui/QueryForm.cpp` | 只读网格挂排序/筛选框/导出按钮/复制/大值查看。 |
| `src/ui/MainWindow.h/.cpp` | 注册历史 dock、菜单项、把「送回 SQL」接到当前 QueryForm。 |
| `src/ui/OptionsDialog.cpp` | 历史保留条数/天数配置项。 |
| `jookkit.pro` | 加入新增源文件/头文件。 |
| `test.pro` | 加入 `Exporter`、`HistoryStore` 及其测试。 |

测试(`frontend/tests/`):`tst_exporter.cpp`、`tst_historystore.cpp`。

执行顺序:先纯逻辑(Part 1 Exporter → Part 2 HistoryStore),再后端钩子(Part 3),再 UI(Part 4 历史面板 → Part 5 网格增强+导出 → Part 6 集成与配置)。每个 Part 结束都是可编译、可提交的状态。

---

## Part 1:Exporter(纯逻辑,TDD)

数据模型约定:`headers` 为 `QStringList`(列名),`rows` 为 `QList<QStringList>`(每行单元格文本,长度与 headers 一致)。阶段一所有单元格按**文本字符串**处理(不区分 SQL NULL)。

### Task 1.1:CSV 导出

**Files:**
- Create: `frontend/src/export/Exporter.h`
- Create: `frontend/src/export/Exporter.cpp`
- Create: `frontend/tests/tst_exporter.cpp`
- Modify: `frontend/test.pro`

- [ ] **Step 1: 在 test.pro 注册 Exporter 与测试**

把 `test.pro` 的 SOURCES/HEADERS 改为(追加三行):

```
SOURCES += \
    tests/tst_conndata.cpp \
    tests/tst_exporter.cpp \
    src/backend/ConnData.cpp \
    src/backend/BackendClient.cpp \
    src/backend/BackendProcess.cpp \
    src/export/Exporter.cpp

HEADERS += \
    src/backend/ConnData.h \
    src/backend/BackendClient.h \
    src/backend/BackendProcess.h \
    src/backend/FuncId.h \
    src/export/Exporter.h
```

> 注意:`test.pro` 当前只有一个 SOURCES 块和一个 HEADERS 块,直接替换为以上内容。

- [ ] **Step 2: 写失败测试(CSV)**

创建 `frontend/tests/tst_exporter.cpp`:

```cpp
#include <QtTest>
#include "export/Exporter.h"

class TstExporter : public QObject {
    Q_OBJECT
private slots:
    void csvBasic() {
        QStringList headers{"id", "name"};
        QList<QStringList> rows{{"1", "Alice"}, {"2", "Bob"}};
        QByteArray out = Exporter::toCsv(headers, rows, ',', true, false);
        QCOMPARE(QString::fromUtf8(out), QString("id,name\r\n1,Alice\r\n2,Bob\r\n"));
    }

    void csvQuotingAndBom() {
        QStringList headers{"a", "b"};
        QList<QStringList> rows{{"x,y", "he said \"hi\""}, {"line1\nline2", "ok"}};
        QByteArray out = Exporter::toCsv(headers, rows, ',', true, true);
        // BOM 前缀
        QVERIFY(out.startsWith("\xEF\xBB\xBF"));
        QString body = QString::fromUtf8(out.mid(3));
        QCOMPARE(body,
            QString("a,b\r\n\"x,y\",\"he said \"\"hi\"\"\"\r\n\"line1\nline2\",ok\r\n"));
    }

    void csvNoHeader() {
        QStringList headers{"id"};
        QList<QStringList> rows{{"1"}};
        QByteArray out = Exporter::toCsv(headers, rows, '\t', false, false);
        QCOMPARE(QString::fromUtf8(out), QString("1\r\n"));
    }
};

QTEST_MAIN(TstExporter)
#include "tst_exporter.moc"
```

- [ ] **Step 3: 运行测试确认失败**

Run:
```bash
cd frontend && qmake test.pro && make 2>&1 | tail -20
```
Expected: 编译失败(`Exporter.h` 不存在 / `toCsv` 未定义)。

- [ ] **Step 4: 写 Exporter.h**

创建 `frontend/src/export/Exporter.h`:

```cpp
#ifndef JOOKKIT_EXPORTER_H
#define JOOKKIT_EXPORTER_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QList>

// 把表格数据导出为 CSV / JSON / SQL INSERT。纯函数,不依赖 UI。
// headers: 列名;rows: 每行单元格文本(长度=headers.size());单元格按文本处理。
namespace Exporter {
    // sep 分隔符;withHeader 是否输出表头;bom 是否加 UTF-8 BOM。换行固定 CRLF。
    QByteArray toCsv(const QStringList &headers, const QList<QStringList> &rows,
                     QChar sep, bool withHeader, bool bom);

    // 对象数组:[{"col":"val",...}, ...]
    QByteArray toJson(const QStringList &headers, const QList<QStringList> &rows);

    // INSERT INTO <table> (...) VALUES (...);  dbType: "mysql"|"sqlite"|空。
    // batch=true 时多行合并为一条 INSERT 的多组 VALUES。
    QByteArray toInsertSql(const QString &table, const QStringList &headers,
                           const QList<QStringList> &rows,
                           const QString &dbType, bool batch);
}

#endif
```

- [ ] **Step 5: 写 Exporter.cpp 的 toCsv**

创建 `frontend/src/export/Exporter.cpp`(本步先放 toCsv,后续 Task 补 toJson/toInsertSql):

```cpp
#include "export/Exporter.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace {
QString csvField(const QString &v, QChar sep) {
    bool needQuote = v.contains(sep) || v.contains('"') ||
                     v.contains('\n') || v.contains('\r');
    if (!needQuote) return v;
    QString s = v;
    s.replace("\"", "\"\"");
    return "\"" + s + "\"";
}
}

QByteArray Exporter::toCsv(const QStringList &headers, const QList<QStringList> &rows,
                           QChar sep, bool withHeader, bool bom) {
    QString out;
    if (withHeader) {
        QStringList hs;
        for (const QString &h : headers) hs << csvField(h, sep);
        out += hs.join(sep) + "\r\n";
    }
    for (const QStringList &row : rows) {
        QStringList cs;
        for (const QString &c : row) cs << csvField(c, sep);
        out += cs.join(sep) + "\r\n";
    }
    QByteArray bytes = out.toUtf8();
    if (bom) bytes.prepend("\xEF\xBB\xBF", 3);
    return bytes;
}
```

- [ ] **Step 6: 运行测试确认通过**

Run:
```bash
cd frontend && make 2>&1 | tail -5 && QT_QPA_PLATFORM=offscreen ./tst_jookkit
```
Expected: PASS(含 csvBasic / csvQuotingAndBom / csvNoHeader)。

- [ ] **Step 7: 提交**

```bash
git add frontend/src/export/Exporter.h frontend/src/export/Exporter.cpp frontend/tests/tst_exporter.cpp frontend/test.pro
git commit -m "feat(frontend): Exporter CSV 导出(分隔符/引号转义/BOM)"
```

### Task 1.2:JSON 导出

**Files:**
- Modify: `frontend/src/export/Exporter.cpp`
- Modify: `frontend/tests/tst_exporter.cpp`

- [ ] **Step 1: 加失败测试**

在 `tst_exporter.cpp` 的 private slots 内追加:

```cpp
    void jsonBasic() {
        QStringList headers{"id", "name"};
        QList<QStringList> rows{{"1", "Alice"}};
        QByteArray out = Exporter::toJson(headers, rows);
        QJsonDocument doc = QJsonDocument::fromJson(out);
        QVERIFY(doc.isArray());
        QJsonArray arr = doc.array();
        QCOMPARE(arr.size(), 1);
        QCOMPARE(arr.at(0).toObject().value("id").toString(), QString("1"));
        QCOMPARE(arr.at(0).toObject().value("name").toString(), QString("Alice"));
    }
```

并在文件顶部 include 区追加(若缺):

```cpp
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
```

- [ ] **Step 2: 运行确认失败**

Run: `cd frontend && qmake test.pro && make 2>&1 | tail -10`
Expected: 链接/编译失败(`toJson` 未定义)。

- [ ] **Step 3: 实现 toJson**

在 `Exporter.cpp` 末尾追加:

```cpp
QByteArray Exporter::toJson(const QStringList &headers, const QList<QStringList> &rows) {
    QJsonArray arr;
    for (const QStringList &row : rows) {
        QJsonObject o;
        for (int j = 0; j < headers.size(); ++j)
            o.insert(headers.at(j), j < row.size() ? row.at(j) : QString());
        arr.append(o);
    }
    return QJsonDocument(arr).toJson(QJsonDocument::Indented);
}
```

- [ ] **Step 4: 运行确认通过**

Run: `cd frontend && make 2>&1 | tail -5 && QT_QPA_PLATFORM=offscreen ./tst_jookkit`
Expected: PASS。

- [ ] **Step 5: 提交**

```bash
git add frontend/src/export/Exporter.cpp frontend/tests/tst_exporter.cpp
git commit -m "feat(frontend): Exporter JSON 导出"
```

### Task 1.3:SQL INSERT 导出

**Files:**
- Modify: `frontend/src/export/Exporter.cpp`
- Modify: `frontend/tests/tst_exporter.cpp`

- [ ] **Step 1: 加失败测试**

在 `tst_exporter.cpp` 追加:

```cpp
    void insertMysqlPerRow() {
        QStringList headers{"id", "name"};
        QList<QStringList> rows{{"1", "A'B"}, {"2", "C"}};
        QByteArray out = Exporter::toInsertSql("t", headers, rows, "mysql", false);
        QCOMPARE(QString::fromUtf8(out),
            QString("INSERT INTO `t` (`id`, `name`) VALUES ('1', 'A''B');\n"
                    "INSERT INTO `t` (`id`, `name`) VALUES ('2', 'C');\n"));
    }

    void insertSqliteBatch() {
        QStringList headers{"id"};
        QList<QStringList> rows{{"1"}, {"2"}};
        QByteArray out = Exporter::toInsertSql("t", headers, rows, "sqlite", true);
        QCOMPARE(QString::fromUtf8(out),
            QString("INSERT INTO \"t\" (\"id\") VALUES ('1'), ('2');\n"));
    }
```

- [ ] **Step 2: 运行确认失败**

Run: `cd frontend && qmake test.pro && make 2>&1 | tail -10`
Expected: 失败(`toInsertSql` 未定义)。

- [ ] **Step 3: 实现 toInsertSql**

在 `Exporter.cpp` 顶部匿名命名空间追加引号辅助:

```cpp
namespace {
QString quoteIdent(const QString &id, const QString &dbType) {
    if (dbType == "sqlite")
        return "\"" + QString(id).replace("\"", "\"\"") + "\"";
    return "`" + QString(id).replace("`", "``") + "`";   // mysql/默认
}
QString quoteVal(const QString &v) {
    return "'" + QString(v).replace("'", "''") + "'";
}
}
```

> 若上一步已有匿名命名空间,把这两个函数并入同一个 `namespace { ... }`,不要重复开命名空间。

在 `Exporter.cpp` 末尾追加:

```cpp
QByteArray Exporter::toInsertSql(const QString &table, const QStringList &headers,
                                 const QList<QStringList> &rows,
                                 const QString &dbType, bool batch) {
    QStringList cols;
    for (const QString &h : headers) cols << quoteIdent(h, dbType);
    const QString tbl = quoteIdent(table, dbType);
    const QString colClause = "(" + cols.join(", ") + ")";

    auto valuesOf = [](const QStringList &row) {
        QStringList vs;
        for (const QString &c : row) vs << quoteVal(c);
        return "(" + vs.join(", ") + ")";
    };

    QString out;
    if (batch) {
        QStringList tuples;
        for (const QStringList &row : rows) tuples << valuesOf(row);
        if (!tuples.isEmpty())
            out = "INSERT INTO " + tbl + " " + colClause +
                  " VALUES " + tuples.join(", ") + ";\n";
    } else {
        for (const QStringList &row : rows)
            out += "INSERT INTO " + tbl + " " + colClause +
                   " VALUES " + valuesOf(row) + ";\n";
    }
    return out.toUtf8();
}
```

- [ ] **Step 4: 运行确认通过**

Run: `cd frontend && make 2>&1 | tail -5 && QT_QPA_PLATFORM=offscreen ./tst_jookkit`
Expected: PASS。

- [ ] **Step 5: 提交**

```bash
git add frontend/src/export/Exporter.cpp frontend/tests/tst_exporter.cpp
git commit -m "feat(frontend): Exporter SQL INSERT 导出(逐行/批量,标识符随数据源)"
```

---

## Part 2:HistoryStore(纯逻辑,TDD)

历史记录结构与持久化。存配置目录下 `history.jsonl`(每行一个 JSON 对象,追加写)。测试用环境变量隔离配置目录,避免污染真实历史。

### Task 2.1:HistoryEntry 与追加/加载

**Files:**
- Create: `frontend/src/store/HistoryStore.h`
- Create: `frontend/src/store/HistoryStore.cpp`
- Create: `frontend/tests/tst_historystore.cpp`
- Modify: `frontend/test.pro`

- [ ] **Step 1: test.pro 注册**

把 `test.pro` 的 SOURCES/HEADERS 再追加(在 Part 1 基础上):

```
SOURCES += \
    tests/tst_historystore.cpp \
    src/store/HistoryStore.cpp

HEADERS += \
    src/store/HistoryStore.h
```

> qmake 允许多个 `SOURCES +=` 累加,直接在文件末尾追加这两块即可。

- [ ] **Step 2: 写失败测试**

创建 `frontend/tests/tst_historystore.cpp`:

```cpp
#include <QtTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include "store/HistoryStore.h"

class TstHistoryStore : public QObject {
    Q_OBJECT
    QTemporaryDir dir_;
private slots:
    void init() {
        // 把配置目录指向临时目录,隔离测试
        QStandardPaths::setTestModeEnabled(true);
        qputenv("XDG_CONFIG_HOME", dir_.path().toUtf8());
        HistoryStore::clear();
    }

    void appendAndLoad() {
        HistoryEntry e;
        e.sql = "SELECT 1"; e.connId = "c1"; e.db = "d1";
        e.ts = 1000; e.elapsedMs = 5; e.rows = 1; e.ok = true;
        HistoryStore::append(e);

        QList<HistoryEntry> all = HistoryStore::load(100);
        QCOMPARE(all.size(), 1);
        QCOMPARE(all.at(0).sql, QString("SELECT 1"));
        QCOMPARE(all.at(0).rows, 1);
        QCOMPARE(all.at(0).ok, true);
    }

    void loadNewestFirst() {
        HistoryEntry a; a.sql = "A"; a.ts = 1;
        HistoryEntry b; b.sql = "B"; b.ts = 2;
        HistoryStore::append(a);
        HistoryStore::append(b);
        QList<HistoryEntry> all = HistoryStore::load(100);
        QCOMPARE(all.size(), 2);
        QCOMPARE(all.at(0).sql, QString("B"));  // 最新在前
        QCOMPARE(all.at(1).sql, QString("A"));
    }
};

QTEST_MAIN(TstHistoryStore)
#include "tst_historystore.moc"
```

- [ ] **Step 3: 运行确认失败**

Run: `cd frontend && qmake test.pro && make 2>&1 | tail -10`
Expected: 失败(`HistoryStore.h` 不存在)。

- [ ] **Step 4: 写 HistoryStore.h**

创建 `frontend/src/store/HistoryStore.h`:

```cpp
#ifndef JOOKKIT_HISTORYSTORE_H
#define JOOKKIT_HISTORYSTORE_H

#include <QString>
#include <QList>

// 一条 SQL 执行历史(只存文本+元数据,不存结果集)
struct HistoryEntry {
    QString sql;
    QString connId;
    QString db;
    qint64  ts = 0;        // epoch ms
    int     elapsedMs = 0;
    int     rows = 0;      // 影响/返回行数
    bool    ok = true;
    QString error;         // 失败时的错误信息
};

// 持久化为配置目录下 history.jsonl(每行一条 JSON,追加写)
namespace HistoryStore {
    void append(const HistoryEntry &e);            // 追加一条
    QList<HistoryEntry> load(int limit);           // 最新在前,最多 limit 条
    void clear();                                  // 清空
    // 裁剪:仅保留最近 maxCount 条且不早于 maxDays 天(maxDays<=0 表示不按天裁)
    void trim(int maxCount, int maxDays);
}

#endif
```

- [ ] **Step 5: 写 HistoryStore.cpp(append/load/clear)**

创建 `frontend/src/store/HistoryStore.cpp`:

```cpp
#include "store/HistoryStore.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>

namespace {
QString filePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (dir.isEmpty()) dir = QDir::homePath() + "/.jookkit";
    QDir().mkpath(dir);
    return dir + "/history.jsonl";
}

QJsonObject toObj(const HistoryEntry &e) {
    QJsonObject o;
    o.insert("sql", e.sql);
    o.insert("connId", e.connId);
    o.insert("db", e.db);
    o.insert("ts", e.ts);
    o.insert("elapsedMs", e.elapsedMs);
    o.insert("rows", e.rows);
    o.insert("ok", e.ok);
    o.insert("error", e.error);
    return o;
}

HistoryEntry fromObj(const QJsonObject &o) {
    HistoryEntry e;
    e.sql = o.value("sql").toString();
    e.connId = o.value("connId").toString();
    e.db = o.value("db").toString();
    e.ts = (qint64)o.value("ts").toDouble();
    e.elapsedMs = o.value("elapsedMs").toInt();
    e.rows = o.value("rows").toInt();
    e.ok = o.value("ok").toBool(true);
    e.error = o.value("error").toString();
    return e;
}
}

void HistoryStore::append(const HistoryEntry &e) {
    QFile f(filePath());
    if (!f.open(QIODevice::Append | QIODevice::Text)) return;  // 失败不阻断主流程
    QTextStream ts(&f);
    ts.setCodec("UTF-8");
    ts << QString::fromUtf8(QJsonDocument(toObj(e)).toJson(QJsonDocument::Compact))
       << "\n";
}

QList<HistoryEntry> HistoryStore::load(int limit) {
    QList<HistoryEntry> out;
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return out;
    QList<HistoryEntry> all;
    QTextStream ts(&f);
    ts.setCodec("UTF-8");
    while (!ts.atEnd()) {
        const QString line = ts.readLine().trimmed();
        if (line.isEmpty()) continue;
        QJsonParseError err{};
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) continue; // 跳过坏行
        all << fromObj(doc.object());
    }
    // 最新在前
    for (int i = all.size() - 1; i >= 0 && out.size() < limit; --i)
        out << all.at(i);
    return out;
}

void HistoryStore::clear() {
    QFile::remove(filePath());
}
```

- [ ] **Step 6: 运行确认通过**

Run: `cd frontend && qmake test.pro && make 2>&1 | tail -5 && QT_QPA_PLATFORM=offscreen ./tst_jookkit`
Expected: PASS(appendAndLoad / loadNewestFirst)。

- [ ] **Step 7: 提交**

```bash
git add frontend/src/store/HistoryStore.h frontend/src/store/HistoryStore.cpp frontend/tests/tst_historystore.cpp frontend/test.pro
git commit -m "feat(frontend): HistoryStore 追加/加载(history.jsonl,最新在前,坏行容错)"
```

### Task 2.2:裁剪(条数 + 天数)

**Files:**
- Modify: `frontend/src/store/HistoryStore.cpp`
- Modify: `frontend/tests/tst_historystore.cpp`

- [ ] **Step 1: 加失败测试**

在 `tst_historystore.cpp` 追加:

```cpp
    void trimByCount() {
        for (int i = 0; i < 5; ++i) {
            HistoryEntry e; e.sql = QString::number(i); e.ts = i; HistoryStore::append(e);
        }
        HistoryStore::trim(3, 0);          // 只保留最近 3 条
        QList<HistoryEntry> all = HistoryStore::load(100);
        QCOMPARE(all.size(), 3);
        QCOMPARE(all.at(0).sql, QString("4"));  // 最新
        QCOMPARE(all.at(2).sql, QString("2"));
    }

    void trimByDays() {
        HistoryEntry oldE; oldE.sql = "old"; oldE.ts = 1; // 1970,远早于阈值
        HistoryEntry newE; newE.sql = "new";
        newE.ts = QDateTime::currentMSecsSinceEpoch();
        HistoryStore::append(oldE);
        HistoryStore::append(newE);
        HistoryStore::trim(1000, 30);      // 保留最近 30 天
        QList<HistoryEntry> all = HistoryStore::load(100);
        QCOMPARE(all.size(), 1);
        QCOMPARE(all.at(0).sql, QString("new"));
    }
```

并在文件顶部 include 追加:

```cpp
#include <QDateTime>
```

- [ ] **Step 2: 运行确认失败**

Run: `cd frontend && qmake test.pro && make 2>&1 | tail -10`
Expected: 失败(`trim` 未定义)。

- [ ] **Step 3: 实现 trim**

在 `HistoryStore.cpp` 顶部 include 追加:

```cpp
#include <QDateTime>
```

在文件末尾追加:

```cpp
void HistoryStore::trim(int maxCount, int maxDays) {
    // 读全部
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QList<HistoryEntry> all;
    {
        QTextStream ts(&f);
        ts.setCodec("UTF-8");
        while (!ts.atEnd()) {
            const QString line = ts.readLine().trimmed();
            if (line.isEmpty()) continue;
            QJsonParseError err{};
            QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
            if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;
            all << fromObj(doc.object());
        }
    }
    f.close();

    // 按天数过滤
    if (maxDays > 0) {
        const qint64 cutoff =
            QDateTime::currentMSecsSinceEpoch() - (qint64)maxDays * 86400000LL;
        QList<HistoryEntry> kept;
        for (const HistoryEntry &e : all)
            if (e.ts >= cutoff) kept << e;
        all = kept;
    }
    // 按条数保留尾部 maxCount(文件按时间追加,尾部即最新)
    if (maxCount > 0 && all.size() > maxCount)
        all = all.mid(all.size() - maxCount);

    // 重写文件
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;
    QTextStream ts(&f);
    ts.setCodec("UTF-8");
    for (const HistoryEntry &e : all)
        ts << QString::fromUtf8(QJsonDocument(toObj(e)).toJson(QJsonDocument::Compact))
           << "\n";
}
```

- [ ] **Step 4: 运行确认通过**

Run: `cd frontend && make 2>&1 | tail -5 && QT_QPA_PLATFORM=offscreen ./tst_jookkit`
Expected: PASS(trimByCount / trimByDays)。

- [ ] **Step 5: 提交**

```bash
git add frontend/src/store/HistoryStore.cpp frontend/tests/tst_historystore.cpp
git commit -m "feat(frontend): HistoryStore 裁剪(按条数+天数,重写文件)"
```

---

## Part 3:BackendClient 历史钩子

在 `call` 统一出口,对执行类 funcId 记录历史。db 上下文由调用方通过 setter 注入(call 的 request 不一定带 db)。

### Task 3.1:在 call 内记录执行类请求

**Files:**
- Modify: `frontend/src/backend/BackendClient.h`
- Modify: `frontend/src/backend/BackendClient.cpp`
- Modify: `frontend/jookkit.pro`

- [ ] **Step 1: jookkit.pro 加入新模块**

在 `jookkit.pro` 的 SOURCES 追加(放在 `src/store/FavoriteStore.cpp` 后):

```
    src/store/HistoryStore.cpp \
    src/export/Exporter.cpp \
```

HEADERS 追加:

```
    src/store/HistoryStore.h \
    src/export/Exporter.h \
```

- [ ] **Step 2: BackendClient.h 加历史开关与 db 上下文**

在 `BackendClient` public 区(`call` 声明后)追加:

```cpp
    // 历史记录上下文:调用方在切换连接/库时更新,call() 记录时带上。
    void setHistoryContext(const QString &connId, const QString &db);
```

在 private 区(`int port_ = 0;` 后)追加:

```cpp
    QString histConnId_;
    QString histDb_;
```

- [ ] **Step 3: BackendClient.cpp 实现记录**

在 `BackendClient.cpp` 顶部 include 追加:

```cpp
#include "backend/FuncId.h"
#include "store/HistoryStore.h"
#include <QElapsedTimer>
#include <QDateTime>
```

加 setter(放在 `setPort` 实现附近):

```cpp
void BackendClient::setHistoryContext(const QString &connId, const QString &db) {
    histConnId_ = connId;
    histDb_ = db;
}
```

在 `call` 实现里,把请求发出前后用计时包起来,并在返回前记录。具体做法:在 `call` 函数体最开始加:

```cpp
    QElapsedTimer _timer; _timer.start();
    const int _funcId = request.value("funcId").toInt();
```

在 `call` 计算出 `Result`(设其变量名为已有的返回结构,假设为 `Result r;`)、即将 `return` 之前,插入:

```cpp
    // 仅对执行类请求记录历史(查询/写操作),元数据请求不记
    if (_funcId == FuncId::EXEC_SQL || _funcId == FuncId::INSERT_ROW ||
        _funcId == FuncId::UPDATE_ROW || _funcId == FuncId::DELETE_ROW) {
        HistoryEntry _h;
        _h.sql = request.value("sql").toString();
        if (_h.sql.isEmpty()) _h.sql = request.value("query").toString();
        _h.connId = histConnId_;
        _h.db = histDb_;
        _h.ts = QDateTime::currentMSecsSinceEpoch();
        _h.elapsedMs = (int)_timer.elapsed();
        _h.ok = r.ok;
        _h.error = r.ok ? QString() : (r.errorMessage.isEmpty() ? r.errorCode : r.errorMessage);
        _h.rows = r.data.value("rows").isArray()
                    ? r.data.value("rows").toArray().size()
                    : r.data.value("affected").toInt();
        HistoryStore::append(_h);
    }
    return r;
```

> 实现注意:`call` 现有代码可能多处 `return`。把上面记录逻辑提成一个 lambda `auto record = [&](const Result &r){...};` 并在每个 return 前 `record(r);`,或重构为单一出口。务必保证每条返回路径(含 transportFailed)都经过记录。读 `BackendClient.cpp` 现有 `call` 实现后选最小改动方式。
>
> 字段名核对:`request` 里 SQL 的键名以 `QueryForm.cpp`/`TableDataForm.cpp` 实际构造为准(grep `"funcId"` 附近的 `insert(`)。若不是 `sql`/`query`,改成实际键名。`r.data` 里行数键名以后端返回为准(grep 后端 `ExecSqlHandler` 的输出键)。

- [ ] **Step 4: 编译主程序确认通过**

Run: `cd frontend && qmake jookkit.pro && make -j3 2>&1 | tail -15`
Expected: 编译成功(无未定义符号)。

- [ ] **Step 5: 单测仍绿**

Run: `cd frontend && qmake test.pro && make 2>&1 | tail -5 && QT_QPA_PLATFORM=offscreen ./tst_jookkit`
Expected: PASS(BackendClient 已在 test.pro,需确保 HistoryStore 也在——Part 2 已加)。

- [ ] **Step 6: 提交**

```bash
git add frontend/src/backend/BackendClient.h frontend/src/backend/BackendClient.cpp frontend/jookkit.pro
git commit -m "feat(frontend): BackendClient 对执行类请求记录 SQL 历史"
```

### Task 3.2:QueryForm/TableDataForm 注入 db 上下文

**Files:**
- Modify: `frontend/src/ui/QueryForm.cpp`
- Modify: `frontend/src/ui/TableDataForm.cpp`

- [ ] **Step 1: QueryForm 切换连接/库时更新上下文**

在 `QueryForm.cpp` 的 `onConnChanged` 末尾、以及 `runText`/`run` 真正发请求前,调用:

```cpp
    client_->setHistoryContext(currentConn().connId, dbCombo_->currentText());
```

放在 `client_->call(req)` 之前一行最稳妥(确保 db 与本次执行一致)。

- [ ] **Step 2: TableDataForm 同样注入**

在 `TableDataForm.cpp` 每次 `client_->call(...)` 执行写操作前(`onItemChanged`/`saveNewRows`/`deleteSelectedRow` 里),加:

```cpp
    client_->setHistoryContext(connId_, db_);
```

- [ ] **Step 3: 编译确认通过**

Run: `cd frontend && qmake jookkit.pro && make -j3 2>&1 | tail -10`
Expected: 成功。

- [ ] **Step 4: 提交**

```bash
git add frontend/src/ui/QueryForm.cpp frontend/src/ui/TableDataForm.cpp
git commit -m "feat(frontend): 执行前注入历史上下文(连接/库)"
```

---

## Part 4:历史面板(HistoryPane + dock)

### Task 4.1:HistoryPane 组件

**Files:**
- Create: `frontend/src/ui/HistoryPane.h`
- Create: `frontend/src/ui/HistoryPane.cpp`
- Modify: `frontend/jookkit.pro`

- [ ] **Step 1: jookkit.pro 注册**

SOURCES 追加 `src/ui/HistoryPane.cpp \`,HEADERS 追加 `src/ui/HistoryPane.h \`(放在 `src/ui/` 区)。

- [ ] **Step 2: 写 HistoryPane.h**

创建 `frontend/src/ui/HistoryPane.h`:

```cpp
#ifndef JOOKKIT_HISTORYPANE_H
#define JOOKKIT_HISTORYPANE_H

#include <QWidget>
#include "store/HistoryStore.h"

class QLineEdit;
class QListWidget;

// SQL 执行历史面板:搜索框 + 列表;双击发出 sqlChosen(把 SQL 送回编辑器)。
class HistoryPane : public QWidget {
    Q_OBJECT
public:
    explicit HistoryPane(QWidget *parent = nullptr);
    void refresh();                      // 重新从 HistoryStore 加载

signals:
    void sqlChosen(const QString &sql);  // 双击或右键「送回编辑器」

private slots:
    void applyFilter(const QString &text);
    void onItemActivated();
    void showMenu(const QPoint &pos);

private:
    void rebuild();                      // 用 entries_ 重建列表(受 filter_ 影响)

    QLineEdit *search_;
    QListWidget *list_;
    QList<HistoryEntry> entries_;        // 最新在前
    QString filter_;
};

#endif
```

- [ ] **Step 3: 写 HistoryPane.cpp**

创建 `frontend/src/ui/HistoryPane.cpp`:

```cpp
#include "ui/HistoryPane.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>

HistoryPane::HistoryPane(QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    search_ = new QLineEdit(this);
    search_->setPlaceholderText("搜索 SQL …");
    list_ = new QListWidget(this);
    list_->setContextMenuPolicy(Qt::CustomContextMenu);
    lay->addWidget(search_);
    lay->addWidget(list_);

    connect(search_, &QLineEdit::textChanged, this, &HistoryPane::applyFilter);
    connect(list_, &QListWidget::itemActivated, this, &HistoryPane::onItemActivated);
    connect(list_, &QListWidget::itemDoubleClicked, this, &HistoryPane::onItemActivated);
    connect(list_, &QListWidget::customContextMenuRequested, this, &HistoryPane::showMenu);

    refresh();
}

void HistoryPane::refresh() {
    entries_ = HistoryStore::load(1000);
    rebuild();
}

void HistoryPane::applyFilter(const QString &text) {
    filter_ = text;
    rebuild();
}

void HistoryPane::rebuild() {
    list_->clear();
    for (const HistoryEntry &e : entries_) {
        if (!filter_.isEmpty() && !e.sql.contains(filter_, Qt::CaseInsensitive))
            continue;
        const QString when = QDateTime::fromMSecsSinceEpoch(e.ts).toString("MM-dd HH:mm:ss");
        const QString flag = e.ok ? "✓" : "✗";
        QString oneLine = e.sql;
        oneLine.replace('\n', ' ');
        if (oneLine.size() > 80) oneLine = oneLine.left(80) + "…";
        auto *it = new QListWidgetItem(
            QString("%1 [%2] %3").arg(flag, when, oneLine), list_);
        it->setData(Qt::UserRole, e.sql);             // 完整 SQL
        it->setToolTip(e.ok ? e.sql : (e.sql + "\n-- 错误: " + e.error));
    }
}

void HistoryPane::onItemActivated() {
    auto *it = list_->currentItem();
    if (it) emit sqlChosen(it->data(Qt::UserRole).toString());
}

void HistoryPane::showMenu(const QPoint &pos) {
    auto *it = list_->itemAt(pos);
    QMenu menu(this);
    QAction *send = menu.addAction("送回编辑器");
    QAction *copy = menu.addAction("复制 SQL");
    menu.addSeparator();
    QAction *clr = menu.addAction("清空历史");
    send->setEnabled(it); copy->setEnabled(it);
    QAction *chosen = menu.exec(list_->mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == send && it) emit sqlChosen(it->data(Qt::UserRole).toString());
    else if (chosen == copy && it)
        QApplication::clipboard()->setText(it->data(Qt::UserRole).toString());
    else if (chosen == clr) { HistoryStore::clear(); refresh(); }
}
```

- [ ] **Step 4: 编译确认通过**

Run: `cd frontend && qmake jookkit.pro && make -j3 2>&1 | tail -10`
Expected: 成功。

- [ ] **Step 5: 提交**

```bash
git add frontend/src/ui/HistoryPane.h frontend/src/ui/HistoryPane.cpp frontend/jookkit.pro
git commit -m "feat(frontend): HistoryPane 历史面板(搜索/双击送回/右键复制清空)"
```

### Task 4.2:MainWindow 挂载历史 dock

**Files:**
- Modify: `frontend/src/ui/MainWindow.h`
- Modify: `frontend/src/ui/MainWindow.cpp`

- [ ] **Step 1: 读 MainWindow 现状**

Run: `grep -n 'addDockWidget\|QDockWidget\|menuBar\|QMenu\|currentQueryForm\|ContentWidget\|client_\|backend' frontend/src/ui/MainWindow.cpp | head -40`
据此找到:菜单创建处、ContentWidget/当前 QueryForm 的获取方式、BackendClient 实例。

- [ ] **Step 2: MainWindow.h 加成员**

在 `MainWindow.h` private 区加:

```cpp
    class HistoryPane *historyPane_ = nullptr;
```

并在文件顶部加前置声明(class 区上方):`class HistoryPane;`(若已用上面的内联 class 写法则可省)。

- [ ] **Step 3: MainWindow.cpp 创建 dock + 菜单项**

在 `MainWindow.cpp` include 追加:

```cpp
#include "ui/HistoryPane.h"
#include <QDockWidget>
```

在构造函数(UI 初始化完成后)追加:

```cpp
    historyPane_ = new HistoryPane(this);
    auto *histDock = new QDockWidget("SQL 历史", this);
    histDock->setObjectName("historyDock");
    histDock->setWidget(historyPane_);
    addDockWidget(Qt::RightDockWidgetArea, histDock);
    histDock->hide();   // 默认隐藏

    // 「送回编辑器」:把历史 SQL 放进当前查询页编辑器
    connect(historyPane_, &HistoryPane::sqlChosen, this, [this](const QString &sql) {
        // TODO-CONNECT: 用 MainWindow 实际「获取当前 QueryForm」的方法填充
        // 例如:if (auto *qf = currentQueryForm()) qf->setSql(sql);
    });
```

> 在 Step 1 grep 出当前 QueryForm 的获取方式后,把上面 lambda 内补成真实调用(`QueryForm` 已有 `setSql(const QString&)`)。若 MainWindow 无现成「当前 QueryForm」获取途径,在 ContentWidget 上加一个 `QueryForm* currentQueryForm() const` 返回当前 tab 的 QueryForm(若当前 tab 不是 QueryForm 则返回 nullptr,此时新建一个查询页再 setSql)。

在「视图」菜单(若无则在菜单栏新建「视图」菜单)加切换项:

```cpp
    // 找到/创建视图菜单后:
    viewMenu->addAction(histDock->toggleViewAction());  // 复用 dock 自带的显隐切换
    // 打开面板时刷新一次
    connect(histDock, &QDockWidget::visibilityChanged, this, [this](bool v){
        if (v && historyPane_) historyPane_->refresh();
    });
```

- [ ] **Step 4: 编译并手动验证**

Run: `cd frontend && qmake jookkit.pro && make -j3 2>&1 | tail -10`
Expected: 成功。手动:启动程序 → 视图菜单勾选「SQL 历史」→ 执行一条查询 → 面板能搜到、双击 SQL 回到编辑器。

```bash
QT_QPA_PLATFORM=offscreen ./jookkit & sleep 3; kill %1 2>/dev/null   # 仅冒烟:能起不崩
```

- [ ] **Step 5: 提交**

```bash
git add frontend/src/ui/MainWindow.h frontend/src/ui/MainWindow.cpp
git commit -m "feat(frontend): 主窗口挂载 SQL 历史 dock + 视图菜单切换 + 送回编辑器"
```

---

## Part 5:结果网格增强 + 导出入口(QueryForm)

只在**只读结果网格**(`QueryForm::grid_`)做。可编辑网格(TableDataForm)不动。

### Task 5.1:GridUtils 提取数据(纯逻辑可测)

**Files:**
- Create: `frontend/src/ui/GridUtils.h`
- Create: `frontend/src/ui/GridUtils.cpp`
- Modify: `frontend/jookkit.pro`
- Modify: `frontend/test.pro`
- Modify: `frontend/tests/tst_exporter.cpp`(复用测试文件加一组 GridUtils 测试)

- [ ] **Step 1: pro 注册**

`jookkit.pro`:SOURCES 加 `src/ui/GridUtils.cpp \`,HEADERS 加 `src/ui/GridUtils.h \`。
`test.pro`:SOURCES 加 `src/ui/GridUtils.cpp \`,HEADERS 加 `src/ui/GridUtils.h \`,并在 `test.pro` 顶部确保 `QT += widgets`(GridUtils 依赖 QtWidgets)。当前 `test.pro` 为 `QT += core network testlib`,改为:

```
QT += core gui widgets network testlib
```

- [ ] **Step 2: 写失败测试**

在 `tst_exporter.cpp` 顶部 include 追加:

```cpp
#include <QTableWidget>
#include "ui/GridUtils.h"
```

在 private slots 追加:

```cpp
    void gridExtractVisibleOnly() {
        QTableWidget t;
        t.setColumnCount(2);
        t.setHorizontalHeaderLabels({"id", "name"});
        t.setRowCount(2);
        t.setItem(0, 0, new QTableWidgetItem("1"));
        t.setItem(0, 1, new QTableWidgetItem("Alice"));
        t.setItem(1, 0, new QTableWidgetItem("2"));
        t.setItem(1, 1, new QTableWidgetItem("Bob"));
        t.setRowHidden(1, true);  // 第二行被筛掉

        QStringList headers;
        QList<QStringList> rows;
        GridUtils::extract(&t, /*visibleOnly=*/true, /*selectedOnly=*/false, headers, rows);
        QCOMPARE(headers, QStringList({"id", "name"}));
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows.at(0), QStringList({"1", "Alice"}));
    }
```

- [ ] **Step 3: 运行确认失败**

Run: `cd frontend && qmake test.pro && make 2>&1 | tail -10`
Expected: 失败(`GridUtils.h` 不存在)。

- [ ] **Step 4: 写 GridUtils.h**

创建 `frontend/src/ui/GridUtils.h`:

```cpp
#ifndef JOOKKIT_GRIDUTILS_H
#define JOOKKIT_GRIDUTILS_H

#include <QStringList>
#include <QList>

class QTableWidget;

// 从 QTableWidget 提取表头与单元格文本。
namespace GridUtils {
    // visibleOnly: 跳过被 setRowHidden 隐藏的行(筛选后);
    // selectedOnly: 仅选中行(无选中则视为全部)。
    void extract(const QTableWidget *grid, bool visibleOnly, bool selectedOnly,
                 QStringList &headersOut, QList<QStringList> &rowsOut);
}

#endif
```

- [ ] **Step 5: 写 GridUtils.cpp**

创建 `frontend/src/ui/GridUtils.cpp`:

```cpp
#include "ui/GridUtils.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QSet>

void GridUtils::extract(const QTableWidget *grid, bool visibleOnly, bool selectedOnly,
                        QStringList &headersOut, QList<QStringList> &rowsOut) {
    headersOut.clear();
    rowsOut.clear();
    if (!grid) return;
    const int cols = grid->columnCount();
    for (int j = 0; j < cols; ++j) {
        auto *h = grid->horizontalHeaderItem(j);
        headersOut << (h ? h->text() : QString::number(j));
    }

    QSet<int> selRows;
    if (selectedOnly) {
        const auto ranges = grid->selectedRanges();
        for (const auto &r : ranges)
            for (int row = r.topRow(); row <= r.bottomRow(); ++row) selRows.insert(row);
    }

    for (int i = 0; i < grid->rowCount(); ++i) {
        if (visibleOnly && grid->isRowHidden(i)) continue;
        if (selectedOnly && !selRows.isEmpty() && !selRows.contains(i)) continue;
        QStringList row;
        for (int j = 0; j < cols; ++j) {
            auto *it = grid->item(i, j);
            row << (it ? it->text() : QString());
        }
        rowsOut << row;
    }
}
```

- [ ] **Step 6: 运行确认通过**

Run: `cd frontend && qmake test.pro && make 2>&1 | tail -5 && QT_QPA_PLATFORM=offscreen ./tst_jookkit`
Expected: PASS。

- [ ] **Step 7: 提交**

```bash
git add frontend/src/ui/GridUtils.h frontend/src/ui/GridUtils.cpp frontend/jookkit.pro frontend/test.pro frontend/tests/tst_exporter.cpp
git commit -m "feat(frontend): GridUtils 从表格提取数据(可见行/选中行)"
```

### Task 5.2:QueryForm 排序 + 筛选框

**Files:**
- Modify: `frontend/src/ui/QueryForm.h`
- Modify: `frontend/src/ui/QueryForm.cpp`

- [ ] **Step 1: QueryForm.h 加筛选框成员与槽**

在 `QueryForm.h`:`class QLineEdit;` 前置声明(若无);private 区加:

```cpp
    QLineEdit *filterEdit_ = nullptr;
```

private slots 加:

```cpp
    void applyGridFilter(const QString &text);
```

- [ ] **Step 2: 构造里加筛选框、开启排序**

在 `QueryForm.cpp` include 追加 `#include <QLineEdit>`。在创建 `grid_` 之后:

```cpp
    grid_->setSortingEnabled(true);   // 只读结果可点表头排序
```

在网格上方布局插入筛选框(放在 grid_ 所在布局上方,具体按现有布局变量名调整):

```cpp
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText("筛选当前结果(仅当前已加载数据)…");
    // 把 filterEdit_ 加到结果区布局顶部:<layout>->insertWidget(<idx>, filterEdit_);
    connect(filterEdit_, &QLineEdit::textChanged, this, &QueryForm::applyGridFilter);
```

> `showResult` 每次重建网格前会 `setSortingEnabled` 影响插入顺序:在 `showResult` 填充数据**期间先 `grid_->setSortingEnabled(false)`,填充完再 `setSortingEnabled(true)`**,否则边插边排会错位。在 `showResult` 开头加 `grid_->setSortingEnabled(false);`,结尾加 `grid_->setSortingEnabled(true);`。

- [ ] **Step 3: 实现筛选槽**

在 `QueryForm.cpp` 追加:

```cpp
void QueryForm::applyGridFilter(const QString &text) {
    for (int i = 0; i < grid_->rowCount(); ++i) {
        bool match = text.isEmpty();
        for (int j = 0; !match && j < grid_->columnCount(); ++j) {
            auto *it = grid_->item(i, j);
            if (it && it->text().contains(text, Qt::CaseInsensitive)) match = true;
        }
        grid_->setRowHidden(i, !match);
    }
}
```

- [ ] **Step 4: 编译 + 冒烟**

Run: `cd frontend && qmake jookkit.pro && make -j3 2>&1 | tail -10`
Expected: 成功。手动:执行查询 → 点表头能排序 → 筛选框输入能隐藏不匹配行。

- [ ] **Step 5: 提交**

```bash
git add frontend/src/ui/QueryForm.h frontend/src/ui/QueryForm.cpp
git commit -m "feat(frontend): 结果网格表头排序 + 关键字筛选(仅当前数据)"
```

### Task 5.3:导出按钮 + 复制 + 大值查看

**Files:**
- Modify: `frontend/src/ui/QueryForm.h`
- Modify: `frontend/src/ui/QueryForm.cpp`

- [ ] **Step 1: QueryForm.h 加槽**

private slots 追加:

```cpp
    void exportResult();
    void copySelection();
    void showCellValue(int row, int col);
```

- [ ] **Step 2: 导出实现**

`QueryForm.cpp` include 追加:

```cpp
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include "ui/GridUtils.h"
#include "export/Exporter.h"
```

实现导出(弹文件对话框,按扩展名选格式;范围:有选中→仅选中,否则可见行):

```cpp
void QueryForm::exportResult() {
    if (grid_->rowCount() == 0) {
        QMessageBox::information(this, "导出", "没有可导出的数据。");
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, "导出结果", QString(),
        "CSV (*.csv);;JSON (*.json);;SQL INSERT (*.sql)");
    if (path.isEmpty()) return;

    const bool selectedOnly = !grid_->selectedRanges().isEmpty();
    QStringList headers; QList<QStringList> rows;
    GridUtils::extract(grid_, /*visibleOnly=*/true, selectedOnly, headers, rows);

    QByteArray bytes;
    if (path.endsWith(".json", Qt::CaseInsensitive))
        bytes = Exporter::toJson(headers, rows);
    else if (path.endsWith(".sql", Qt::CaseInsensitive))
        bytes = Exporter::toInsertSql("exported_table", headers, rows, QString(), true);
    else
        bytes = Exporter::toCsv(headers, rows, ',', true, /*bom=*/true);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "导出", "无法写入文件:" + path);
        return;
    }
    f.write(bytes);
    QMessageBox::information(this, "导出",
        QString("已导出 %1 行到\n%2").arg(rows.size()).arg(path));
}
```

实现复制为 TSV:

```cpp
void QueryForm::copySelection() {
    const bool selectedOnly = !grid_->selectedRanges().isEmpty();
    QStringList headers; QList<QStringList> rows;
    GridUtils::extract(grid_, true, selectedOnly, headers, rows);
    QByteArray tsv = Exporter::toCsv(headers, rows, '\t', true, false);
    QApplication::clipboard()->setText(QString::fromUtf8(tsv));
}
```

实现大值查看:

```cpp
void QueryForm::showCellValue(int row, int col) {
    auto *it = grid_->item(row, col);
    if (!it) return;
    QDialog dlg(this);
    dlg.setWindowTitle("单元格内容");
    dlg.resize(500, 360);
    auto *lay = new QVBoxLayout(&dlg);
    auto *edit = new QPlainTextEdit(&dlg);
    edit->setReadOnly(true);
    edit->setPlainText(it->text());
    lay->addWidget(edit);
    dlg.exec();
}
```

- [ ] **Step 3: 接线(按钮 + 快捷键 + 双击)**

在构造函数里:加「导出」按钮到结果区工具条(按现有按钮创建方式),`connect(... &QueryForm::exportResult)`;
双击查看:`connect(grid_, &QTableWidget::cellDoubleClicked, this, &QueryForm::showCellValue);`
复制快捷键:

```cpp
    auto *copyAct = new QAction(this);
    copyAct->setShortcut(QKeySequence::Copy);
    connect(copyAct, &QAction::triggered, this, &QueryForm::copySelection);
    grid_->addAction(copyAct);
    grid_->setContextMenuPolicy(Qt::ActionsContextMenu);
    auto *expAct = new QAction("导出…", this);
    connect(expAct, &QAction::triggered, this, &QueryForm::exportResult);
    grid_->addAction(expAct);
```

include 追加 `#include <QAction>`、`#include <QKeySequence>`。

- [ ] **Step 4: 编译 + 冒烟**

Run: `cd frontend && qmake jookkit.pro && make -j3 2>&1 | tail -10`
Expected: 成功。手动:查询后右键网格有「导出…」;选区 Ctrl+C 可粘到 Excel;双击大值弹窗。

- [ ] **Step 5: 提交**

```bash
git add frontend/src/ui/QueryForm.h frontend/src/ui/QueryForm.cpp
git commit -m "feat(frontend): 结果导出(CSV/JSON/SQL)+ 复制TSV + 大值查看"
```

---

## Part 6:配置项与收尾

### Task 6.1:OptionsDialog 历史保留配置 + 启动裁剪

**Files:**
- Modify: `frontend/src/ui/OptionsDialog.cpp`
- Modify: `frontend/src/ui/MainWindow.cpp`

- [ ] **Step 1: 读 OptionsDialog 现状**

Run: `grep -n 'QSettings\|addRow\|QSpinBox\|QFormLayout\|save\|load' frontend/src/ui/OptionsDialog.cpp | head -30`
据此了解其设置项存取方式(大概率 `QSettings`)。

- [ ] **Step 2: 加两个数字项**

在 `OptionsDialog.cpp` 表单里加(沿用现有 QSettings 键风格):

```cpp
    // 历史:保留条数 / 天数
    auto *histCount = new QSpinBox(this);
    histCount->setRange(0, 100000);
    histCount->setValue(settings.value("history/maxCount", 1000).toInt());
    auto *histDays = new QSpinBox(this);
    histDays->setRange(0, 3650);
    histDays->setValue(settings.value("history/maxDays", 30).toInt());
    // 加到表单:form->addRow("历史保留条数", histCount); form->addRow("历史保留天数", histDays);
```

保存处:

```cpp
    settings.setValue("history/maxCount", histCount->value());
    settings.setValue("history/maxDays", histDays->value());
```

include 加 `#include <QSpinBox>`、`#include <QSettings>`(若缺)。

- [ ] **Step 3: 启动时按配置裁剪历史**

在 `MainWindow.cpp` 构造函数(创建 historyPane_ 前)加:

```cpp
    {
        QSettings s;
        HistoryStore::trim(s.value("history/maxCount", 1000).toInt(),
                           s.value("history/maxDays", 30).toInt());
    }
```

include 加 `#include <QSettings>`、`#include "store/HistoryStore.h"`。

- [ ] **Step 4: 编译 + 冒烟**

Run: `cd frontend && qmake jookkit.pro && make -j3 2>&1 | tail -10`
Expected: 成功。手动:选项里能改两个值;重启后历史按设置裁剪。

- [ ] **Step 5: 提交**

```bash
git add frontend/src/ui/OptionsDialog.cpp frontend/src/ui/MainWindow.cpp
git commit -m "feat(frontend): 历史保留条数/天数配置 + 启动裁剪"
```

### Task 6.2:全量回归 + README

**Files:**
- Modify: `README.md`

- [ ] **Step 1: 跑全部单测**

Run:
```bash
cd frontend && qmake test.pro && make 2>&1 | tail -5 && QT_QPA_PLATFORM=offscreen ./tst_jookkit
```
Expected: 全 PASS(ConnData + Exporter + HistoryStore + GridUtils)。

- [ ] **Step 2: 主程序构建冒烟**

Run:
```bash
cd frontend && qmake jookkit.pro && make -j3 2>&1 | tail -5 && QT_QPA_PLATFORM=offscreen ./jookkit & sleep 3; kill %1 2>/dev/null
```
Expected: 构建成功、能启动不崩。

- [ ] **Step 3: README 更新「主要功能」**

在 `README.md` 「主要功能」列表追加:

```markdown
- **SQL 执行历史**:自动记录执行过的 SQL(连接/库/耗时/行数/成败),面板内搜索、双击送回编辑器、可配置保留条数与天数
- **结果网格增强**:表头排序、关键字筛选(当前数据)、复制为 TSV、单元格大值查看
- **结果导出**:把查询结果(全部/选中/筛选后)导出为 CSV(UTF-8 BOM)、JSON、SQL INSERT
```

- [ ] **Step 4: 提交**

```bash
git add README.md
git commit -m "docs: README 补充阶段一新功能(SQL历史/网格增强/导出)"
```

---

## 自检对照(Spec → 计划)

- A SQL 历史:字段(3.1)→Task2.1;拦截点(3.2)→Task3.1;存储/滚动(3.3)→Task2.1/2.2/6.1;UI(3.4)→Task4.1/4.2。✓
- B 网格增强:排序(4.1,仅只读)→Task5.2;筛选(4.2)→Task5.2;复制(4.3)→Task5.3;大值(4.4)→Task5.3。✓
- C 导出:入口/范围(5.1)→Task5.3;格式/选项(5.2)→Task1.1-1.3;数据来源(5.3,网格已加载/流式写)→Task5.1+5.3。✓
- 不做项(.xlsx/整表流式/导入):计划未引入,符合。✓
- 测试策略(§7):Exporter/HistoryStore/GridUtils 均有单测;历史钩子在 Part3 编译+冒烟覆盖。✓
- 验收标准(§9):各条对应 Task5/4/6 的冒烟步骤。✓
