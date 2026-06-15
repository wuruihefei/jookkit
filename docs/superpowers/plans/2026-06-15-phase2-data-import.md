# 阶段二:数据导入(CSV/JSON → 已存在的表)实现计划

```metadata
status: approved
scope: medium
spec: docs/superpowers/specs/2026-06-15-phase2-data-import-design.md
```

## Problem Statement

JookKit 目前能导出(CSV/JSON/INSERT SQL)和手工执行 SQL,但要把外部 CSV/JSON 文件灌进一张**已存在**的表,只能手写 INSERT。本计划实现"右键表 → 导入数据到此表",支持列名自动匹配+手动调整、空值→NULL、坏行跳过并报告,面向几千~几万行的中小文件(内存版)。

## Solution Overview

纯前端实现(Qt5/C++17),不改后端协议。读文件 → 解析成 `QList<QStringList>`(用 `QString::isNull()` 表示 SQL NULL)→ 列映射 → 复用升级后的 `Exporter` 生成批量 INSERT,经 `EXEC_SQL` 发送;某批失败时回退到该批逐行执行以定位坏行,最终汇总失败清单。导入期间临时关闭 SQL 历史记录。新增 4 个无 UI 依赖、可独立单测的核心模块 + 1 个对话框,严格 TDD。

执行顺序(每步先红后绿,提交后再下一步):
**Exporter NULL 升级 → CsvReader → JsonReader → ColumnMapping → ImportRunner → BackendClient 历史开关 → ImportDialog/右键集成**

## 关键约束(来自用户,逐字保留)

- "先做简单可靠的,事务以后再说。改后端的事尽量放后面。"
- "空就当 NULL,数字日期列才不会报错。"
- "编码默认自动检测,但要能手动改成 GBK。预览很重要,能看到才放心。"
- "几万行顶天了,先做内存版,百万行那种场景我这边用不上。"
- NULL 表示法:贯穿全链路用 `QString()`(`isNull()==true`)= SQL NULL;`QString("")`(空串但非 null)= 空字符串字面量。

## 已核实的集成点(写代码时直接用)

| 用途 | 位置 | 备注 |
|---|---|---|
| 表项右键菜单(Table 分支) | `frontend/src/ui/ObjectTree.cpp:46-51` | 已有"打开数据"/"查看结构",模式 `menu.addAction(icon, tr("..."), this, [=]{ emit sig(connId, db, table); });` |
| 获取完整连接(含 dbType) | `ObjectTree::allConnections()` `ObjectTree.cpp:184-191` | 遍历找 `c.connId == connId` 取 `c.type` |
| 打开表数据的范式 | `ContentWidget::openTableData()` `ContentWidget.cpp:131-143` | 先 `tree_->ensureOpen(connId)`,再查 dbType,再 `new TableDataForm(...)` |
| 表结构请求 | `TableDataForm.cpp` 用 `FuncId::DESCRIBE_TABLE` | 请求字段 `funcId/connId/db/table`;响应 `data.columns[].{name,type,nullable}` + `data.primaryKeys[]`。**注意:不是 spec 里写的 TABLE_META=9,以 TableDataForm 实际用的为准** |
| 历史记录钩子 | `BackendClient.cpp` lambda `_recordHistory`(白名单 `_isExecClass`,三处调用)+ `.h` 私有成员区 | 加 `setHistoryEnabled(bool)` + `bool historyEnabled_=true;`,在 `_recordHistory` 开头 `if(!_isExecClass || !historyEnabled_) return;` |
| 现状 quoteVal | `frontend/src/export/Exporter.cpp:22-24` | 当前**不区分 NULL**,无脑加引号,需升级 |

---

## Implementation Phases

### Phase 1: Exporter NULL-aware 升级(最先做,且"改后端"无关,风险最低)

**Goal:** `quoteVal` 能把 `QString::isNull()` 的值输出为裸 `NULL`,其余照旧转义加引号;`toInsertSql` 因此天然支持 NULL。这是导入链路的 SQL 生成基石。

**Tasks:**

- [ ] Task 1.1:在 `frontend/tests/` 下建立 `tst_exporter`(若 `main_tests.cpp` 是统一入口则在其中加用例)。**先写失败测试**:
  - `toInsertSql` 单行,某列值为 `QString()` → 生成的 SQL 该列为 `NULL`(无引号),空串 `QString("")` → `''`。
  - 含单引号的值仍正确转义(`O'Brien` → `'O''Brien'`)。
  - batch 模式多行,混合 NULL/空串/普通值,断言整条 SQL 文本。
- [ ] Task 1.2:升级 `Exporter.cpp` 的 `quoteVal`:`return v.isNull() ? QStringLiteral("NULL") : "'" + QString(v).replace("'","''") + "'";`。`valuesOf` 里 `j >= row.size()` 缺列的占位也应是 `QString()`(已是),从而缺列→NULL。绿。
- [ ] Task 1.3:把 `tst_exporter` 源文件登记进 `test.pro` 的 `SOURCES`(`Exporter.cpp` 已在 line 13)。

**Validation:** `cd frontend && qmake test.pro && make && ./tst_jookkit` 全绿;NULL/空串/转义三类断言通过。

**风险/注意:** `toCsv`/`toJson` 是否也要 null-aware?**本阶段不动它们**(导出方向当前行为可接受,改动会扩散到 GridUtils 测试)。仅 `quoteVal`(导入用)升级。若后续导出也要区分,另开计划。

---

### Phase 2: CsvReader(解析 + 编码检测)

**Goal:** 字节流 → `ParseResult`,自动检测编码(默认)或强制指定(GBK 等),正确处理引号包裹、转义双引号、字段内换行、CRLF、空字段→`QString()`(NULL)。

**接口(来自 spec):**
```cpp
struct CsvOptions { QString forcedCodec; QChar sep = ','; bool firstRowHeader = true; };
struct ParseResult { QStringList headers; QList<QStringList> rows;
                     QVector<int> sourceLines; QString detectedCodec; bool ok = true; QString error; };
ParseResult CsvReader::read(const QByteArray &bytes, const CsvOptions &opt);
```

**Tasks:**

- [ ] Task 2.1:`tst_csvreader` 先写失败测试,覆盖:
  - 基本逗号分隔 + 表头;无表头(`firstRowHeader=false`)时 headers 为空、自动列名留给上层。
  - 引号字段含逗号 `"a,b"`、含转义引号 `"a""b"` → `a"b`、字段内换行 `"line1\nline2"`。
  - 空字段 → `QString()`(断言 `isNull()`),而 `""`(显式空引号)→ `QString("")`(`isEmpty() && !isNull()`)。**这是 NULL 语义的关键区分点。**
  - `sourceLines` 正确(含字段内换行时,行号对应记录起始物理行)。
  - 编码:UTF-8 带/不带 BOM;`forcedCodec="GBK"` 解码中文;自动检测对明显 UTF-8 与明显 GBK 各一例给出 `detectedCodec`。
  - 错误:未闭合引号到 EOF → `ok=false` + `error` 文案。
- [ ] Task 2.2:实现 `frontend/src/import/CsvReader.h/.cpp`。状态机逐字符解析(字段/引号内/引号后三态)。编码检测:优先 BOM;否则用 `QTextCodec` 尝试 UTF-8 严格解码,失败回退 GBK(`QTextCodec::codecForName("GBK")`);`forcedCodec` 非空则跳过检测。绿。
- [ ] Task 2.3:登记进 `test.pro` 与主程序 `jookkit.pro` 的 `SOURCES/HEADERS`。

**Validation:** `tst_jookkit` 全绿;GBK 中文样例正确解码;NULL vs 空串区分断言通过。

**风险:** 编码自动检测无万能解。策略=BOM→UTF-8 严格→GBK 回退,并在预览暴露 `detectedCodec` 让用户能手动改。`QTextCodec` 在 Qt5 core 中可用(`QT += core` 已含),无需额外模块。

---

### Phase 3: JsonReader

**Goal:** JSON 数组(`[{...},{...}]`,对象数组)→ `ParseResult`。headers = 所有对象键的并集(保持首次出现顺序);`null` → `QString()`;缺键 → `QString()`;数字/布尔 → 其文本形式。

**接口:** `ParseResult JsonReader::read(const QByteArray &bytes);`

**Tasks:**

- [ ] Task 3.1:`tst_jsonreader` 先写失败测试:
  - 标准对象数组,键顺序为并集且稳定。
  - `null` 值 → `isNull()`;缺某键的对象该列 → `isNull()`;空串 `""` → 非 null 空串。
  - 数字 `123`/`1.5`、布尔 `true` → `"123"`/`"1.5"`/`"true"`(文本化,交给 DB 隐式转换)。
  - 非数组(顶层是对象)或非对象元素 → `ok=false` + error。
  - 非法 JSON → `ok=false`。
- [ ] Task 3.2:实现 `frontend/src/import/JsonReader.h/.cpp`,用 `QJsonDocument::fromJson`。数字文本化:`QJsonValue::toDouble` 会丢精度,改用对原始 value 判类型——整数用 `QString::number(v.toVariant().toLongLong())` 或直接 `v.toVariant().toString()`(`QVariant` 对 double 整数值可能输出 `1` 而非 `1.0`,需测试锁定行为,以测试断言为准)。绿。
- [ ] Task 3.3:登记进 `.pro`。

**Validation:** `tst_jookkit` 全绿;null/缺键/数字文本化断言通过。

**风险:** 数字精度与文本化格式是最易翻车处——**用测试钉死期望输出**,实现去满足测试,不纠结"理论最佳",符合"简单可靠"。

---

### Phase 4: ColumnMapping(列名匹配 + 应用)

**Goal:** 文件列 → 表列的映射(自动按列名,大小写/首尾空白不敏感),并能把行数据按映射重排成"表列顺序"的行,未映射的表列填 `QString()`(NULL)。

**接口(来自 spec):**
```cpp
// 返回:表列名 -> 文件列下标(未匹配的表列不在 map 中)
QMap<QString,int> ColumnMapping::autoMatch(const QStringList &fileHeaders, const QStringList &tableCols);
void ColumnMapping::apply(const QList<QStringList> &rows, const QStringList &tableCols,
                          const QMap<QString,int> &mapping,
                          QStringList &outCols, QList<QStringList> &outRows);
```

**Tasks:**

- [ ] Task 4.1:`tst_columnmapping` 先写失败测试:
  - 完全同名 → 全映射;大小写不同(`Name` vs `name`)、首尾空格 → 仍匹配。
  - 文件有表里没有的列 → 该文件列被忽略(不出现在 outCols)。
  - 表有文件没有的列 → 不在 mapping 中;`apply` 后该列**不进** outCols(只导用户映射上的列,缺的列让 DB 用默认值/NULL),即 `outCols` 仅含已映射的表列、顺序按 tableCols。
  - `apply`:某行文件侧该格是 `QString()` → 输出仍 `QString()`;文件行长度不足(缺尾列)→ 对应输出 `QString()`。
- [ ] Task 4.2:实现 `frontend/src/import/ColumnMapping.h/.cpp`。`autoMatch` 用规范化键(`trimmed().toLower()`)建文件列索引,遍历表列查。`apply` 按 `tableCols` 顺序,只输出在 mapping 中的列,逐行取 `rows[i][mapping[col]]`(越界→`QString()`)。绿。
- [ ] Task 4.3:登记进 `.pro`。

**Validation:** `tst_jookkit` 全绿。

**设计决策(明确,消歧义):** 只导**用户映射上**的列;未映射的表列完全不出现在 INSERT 的列清单里(交给 DB 默认值/NULL/自增)。这样 NOT NULL 无默认值的列若没映射,会在执行期由 DB 报错并被坏行机制捕获——符合"坏行跳过+报告"。

---

### Phase 5: ImportRunner(批量执行 + 失败回退逐行)

**Goal:** 接收"表列顺序的行",分批生成 INSERT(复用 `Exporter::toInsertSql` batch 模式)经注入的 `Executor` 执行;某批失败则对该批逐行重试,定位坏行进 `failures`;支持进度回调与取消。**不直接依赖 BackendClient**,通过 `std::function` 注入,以便 mock 单测。

**接口(来自 spec):**
```cpp
struct FailedRow { int line; QString reason; QStringList data; };
struct ImportResult { int total=0; int success=0; QVector<FailedRow> failures; bool canceled=false; };
struct ExecOutcome { bool ok; QString error; };
using Executor = std::function<ExecOutcome(const QString &sql)>;
ImportResult ImportRunner::run(const QString &table, const QStringList &cols,
                               const QList<QStringList> &rows, const QString &dbType,
                               Executor exec, int batchSize,
                               std::function<bool(int done,int total)> onProgress);
// onProgress 返回 false 表示用户请求取消
```

**Tasks:**

- [ ] Task 5.1:`tst_importrunner` 先写失败测试,用 **mock Executor**(lambda 按 SQL 内容决定成功/失败):
  - 全成功:`total==success==N`,failures 空,Executor 被调用 `ceil(N/batchSize)` 次(批量)。
  - 单行坏:某批含一条会失败的行 → 该批整体失败 → 逐行重试 → 仅坏行进 failures,`line` 来自 `sourceLines`(需把行号随行传入,见 5.3),其余成功计入 success。
  - 整批全坏:逐行后全部进 failures。
  - 取消:`onProgress` 在第 k 次返回 false → 立即停止,`canceled==true`,后续行不执行。
  - `batchSize==1`:等价纯逐行。
- [ ] Task 5.2:实现 `frontend/src/import/ImportRunner.h/.cpp`。主循环按 `batchSize` 切片 → `Exporter::toInsertSql(table, cols, slice, dbType, /*batch=*/true)` → `exec`;失败则对 slice 每行 `toInsertSql(..., batch=false)` 逐行 `exec`,失败行记 `FailedRow{line, error, rowData}`。每批后调 `onProgress(done,total)`,返回 false 即置 `canceled` 并 break。绿。
- [ ] Task 5.3:**行号传递**:`FailedRow.line` 要对应源文件物理行。两选一(择简):让 `run` 额外接 `const QVector<int>& sourceLines`,或约定调用方在 UI 层用并行数组。**采用前者**:给 `run` 增一个 `sourceLines` 参数(spec 接口据此微调,实现时同步更新 spec 对应签名)。逐行回退时用 `sourceLines[globalIndex]`。
- [ ] Task 5.4:登记进 `.pro`(`ImportRunner.cpp` + `Exporter.cpp` 已在 test.pro)。

**Validation:** `tst_jookkit` 全绿;mock 下批量计数、坏行定位、取消、行号四类断言通过。

**风险(最高):** 批量失败回退是核心价值点。务必测"一批里恰好一条坏行"——批量失败后逐行,成功的也要补计 success(不能因批失败就把整批算失败)。取消语义:**已执行的不回滚**(无事务,符合"事务以后再说"),`canceled=true` 仅表示提前停止。

---

### Phase 6: BackendClient 历史开关(此时才"碰后端客户端",但不改协议)

**Goal:** 导入会产生大量 INSERT,不应污染 SQL 历史。加 `setHistoryEnabled(bool)`,导入期间关、结束(含异常)恢复。

**Tasks:**

- [ ] Task 6.1:`BackendClient.h` 私有区加 `bool historyEnabled_ = true;`,public 加 `void setHistoryEnabled(bool e){ historyEnabled_ = e; }`(内联即可)。
- [ ] Task 6.2:`BackendClient.cpp` 的 `_recordHistory` lambda 开头改为 `if(!_isExecClass || !historyEnabled_) return;`。
- [ ] Task 6.3:此模块行为简单且依赖网络,**不单独写网络单测**;由 Phase 7 的手动验收覆盖(导入后历史面板无 INSERT 刷屏)。在计划中显式记录此取舍。

**Validation:** 编译通过;`tst_jookkit` 仍全绿(BackendClient.cpp 已在 test.pro,确认加成员不破坏现有测试)。

---

### Phase 7: ImportDialog + 右键集成(UI,最后做)

**Goal:** 串起全链路的对话框:选文件 → 选编码(默认"自动",可改 GBK 等)→ 预览前 N 行 → 列映射表格(自动匹配,可手动改/置空)→ 执行(进度条+可取消)→ 结果摘要(成功 N / 失败 M + 失败清单可查看)。从表项右键进入。

**Tasks:**

- [ ] Task 7.1:`ObjectTree`:在 `ObjectTree.cpp:46-51` Table 分支加一项 `menu.addAction(tr("导入数据到此表…"), this, [=]{ emit dataImportRequested(connId, db, table); });`,`.h` 声明信号 `void dataImportRequested(QString connId, QString db, QString table);`(图标可选,先不加避免依赖 Icons 是否有合适图标)。
- [ ] Task 7.2:`ContentWidget`(或现有连接 ObjectTree 信号的同一处,参考 `ContentWidget.cpp:31` connect `tableActivated`)新增 `connect(tree_, &ObjectTree::dataImportRequested, this, &ContentWidget::openImportDialog);`。`openImportDialog` 复用 `openTableData` 的范式:`ensureOpen(connId)` → 从 `allConnections()` 取 dbType → `ImportDialog dlg(client_, connId, db, table, dbType, this); dlg.exec();`。
- [ ] Task 7.3:实现 `frontend/src/ui/ImportDialog.h/.cpp`:
  - 顶部:文件选择(`QFileDialog`,过滤 `*.csv *.json`)+ 编码下拉(`自动 / UTF-8 / GBK / GB18030`,默认"自动")+ 分隔符(CSV 时,默认逗号)。
  - 解析:按扩展名/内容选 `CsvReader`/`JsonReader`,得 `ParseResult`;若 `!ok` 弹错误。编码下拉改变 → 重解析重预览。
  - 预览:`QTableWidget` 显示前 ~50 行;NULL 单元格以淡灰 `<NULL>` 占位区分空串(空串显示为空)。显示 `detectedCodec`。
  - 映射:`QTableWidget`/表单,左列=表列(来自 `DESCRIBE_TABLE`,复用 TableDataForm 取列方式),右列=下拉选文件列(含"<不导入>");初值由 `ColumnMapping::autoMatch`。
  - 取表列:发 `FuncId::DESCRIBE_TABLE` 请求(字段 `connId/db/table`),解析 `data.columns[].name`。
  - 执行:`ColumnMapping::apply` → `client_->setHistoryEnabled(false)` →(`QProgressDialog`)→ `ImportRunner::run(...)`,`Executor` = lambda 调 `client_->call({funcId:EXEC_SQL, connId, db, sql})` 返回 `{r.ok, r.error}`;`onProgress` 更新进度并回传"是否取消"→ `setHistoryEnabled(true)`(放 `try/finally` 等价结构,确保异常也恢复)。
  - 结果:`QMessageBox`/小对话框显示 成功/失败/取消;失败清单(行号+原因)可在文本框查看,提供"复制"。
- [ ] Task 7.4:`ImportDialog.cpp/.h` 登记进 `jookkit.pro`(UI 类**不进** test.pro,无单测)。
- [ ] Task 7.5:手动验收(见下方验收清单)。

**Validation:** 见"验收标准"。UI 层无自动化测试(Qt Widgets 交互测试成本高、价值低),核心逻辑已被 Phase 1-5 单测覆盖,UI 仅做装配。**此取舍明确记录。**

---

## Risks and Mitigations

| 风险 | 缓解 |
|---|---|
| 编码自动检测误判 | BOM→UTF-8严格→GBK 回退;预览暴露 `detectedCodec` + 手动改;用户可见即可纠正 |
| 数字/日期文本化格式不被 DB 接受 | 文本化交 DB 隐式转换;不被接受的行 → 坏行机制捕获并报告,不阻断整体 |
| 批量失败回退逻辑出错(核心) | Phase 5 重点测"单坏行批"+"成功补计 success";mock Executor 全覆盖 |
| 无事务,部分导入后中途失败/取消 → 表里有半截数据 | 符合用户"事务以后再说";结果摘要如实报告成功 N 条;UI 文案提示"已写入的不会回滚" |
| NOT NULL 列未映射 → 全行失败 | 由 DB 报错经坏行机制呈现;映射界面可后续加"必填列未映射"预警(本期不做,YAGNI) |
| 大文件(几十万行)内存/卡顿 | 明确内存版,UI 可加行数上限软提示(本期:仅进度条+可取消,不做分片流式) |

## Testing Strategy

- **单元测试(QtTest,纳入 `test.pro` 的 `tst_jookkit`)**:`tst_exporter`(NULL)、`tst_csvreader`、`tst_jsonreader`、`tst_columnmapping`、`tst_importrunner`(mock Executor)。每个模块严格 TDD:先红后绿。
- **NULL 语义贯穿断言**:凡涉及值的测试都区分 `isNull()`(NULL)与 `isEmpty()&&!isNull()`(空串)。
- **集成/UI**:手动验收,不写自动化 UI 测试(明确取舍)。
- **回归**:每阶段跑全量 `tst_jookkit`,确保 Exporter/BackendClient 改动不破坏 Phase 1 现有测试。
- 命令:`cd frontend && qmake test.pro && make -j && ./tst_jookkit`。

### 手动验收清单(Phase 7)

- [ ] 右键任意表 → 见"导入数据到此表…",点击弹对话框。
- [ ] 选一个 UTF-8 CSV(含中文、含空字段、含带逗号/引号的字段)→ 预览正确,空字段显示 `<NULL>`。
- [ ] 把编码改成 GBK 对一个 GBK 文件 → 中文正常。
- [ ] 列名大小写/空格不同 → 自动匹配上;手动把某列改成"<不导入>"生效。
- [ ] 执行:进度条走动,可取消;成功/失败计数正确。
- [ ] 故意放一行违反类型/NOT NULL → 该行进失败清单(行号对得上源文件),其余成功入库。
- [ ] 导入后看 SQL 历史面板 → **没有**被 INSERT 刷屏(历史开关生效)。
- [ ] JSON 数组文件(含 null、含缺键对象)→ 正确导入,null/缺键入库为 NULL。

## Out of Scope / Future Directions

- 事务/原子导入(失败整体回滚)——用户明确"以后再说"。
- 批量协议(后端 BATCH_INSERT)——本期复用 EXEC_SQL 批量 INSERT 文本,不改后端协议。
- 百万行级流式/分片导入。
- 导入到**新建**表(仅已存在表)。
- SQL 文件导入(用户已可直接执行 SQL)。
- 数据视图工具栏的"导入"按钮(仅右键入口;以后想加再加)。
- 映射界面"必填列未映射"预警、类型预校验。
- 导出方向(toCsv/toJson)的 NULL 区分(本期仅升级 quoteVal/导入方向)。
```
