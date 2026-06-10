# JookKit 设计文档

**日期:** 2026-06-10
**状态:** 设计已确认,待编写实现计划

## 1. 背景与目标

JookKit 是一个**自用**的桌面数据库管理工具,在架构上**复刻 jookdb 的"实际框架实现路径"**——即 jookdb 真正的特色:**C++/Qt 前端 + Java 后端(JDBC)双进程,通过本地 HTTP + JSON 协议通信**。

之所以重新开发而非直接使用 jookdb:

- jookdb 公开的 C++ 前端源码是 **GPLv3**,可自由参考/修改/复用(自用不分发时不触发 copyleft 义务),但源码**残缺**(缺 `utils.h`、`globalutils.cpp`、`conndialog.cpp` 等核心文件),无法直接编译。
- jookdb 的后端 `jookNS.jar` 是**闭源二进制**,无源码、含授权逻辑,既不能复用也不能合法分发。

因此 JookKit 的路径是:**前端基于 jookdb 残缺 C++ 源码补齐,后端从零自写**。授权/试用机制完全不存在(自用工具无此需求)。

### 非目标(YAGNI)

- 不实现授权、试用、激活、Pro 功能门槛。
- MVP 不做导入导出、数据/结构同步、用户管理。
- MVP 不支持 Oracle/达梦/SQLServer 等企业库(留待后续迭代)。

## 2. 范围

### 数据库(MVP)

- **MySQL / MariaDB**(主力,JDBC: `mariadb-java-client`)
- **SQLite**(本地文件库,零服务器,兼作测试主力)

### 功能(MVP)

核心(必做):**连接管理 + SQL 编辑器 + 结果展示**。
加上:**对象树浏览**、**数据网格编辑**、**表结构查看/设计**。

## 3. 架构

### 3.1 进程与通信骨架

```
┌─────────────────────────────┐         ┌──────────────────────────────┐
│   前端 (C++ / Qt5)           │  HTTP   │   后端 (Java, 轻量HTTP)        │
│   主进程,负责全部 UI        │ ──────► │   子进程,负责全部DB访问       │
│                             │ POST    │   127.0.0.1:<随机端口>         │
│   QNetworkAccessManager     │ {funcId}│                              │
│                             │ ◄────── │   JDBC → MySQL/MariaDB/SQLite  │
│                             │  JSON   │                              │
│   ◄── 反向通道(WebSocket)── │         │   (反向回调,如确认/进度)      │
└─────────────────────────────┘         └──────────────────────────────┘
     启动时 QProcess 拉起后端 ──────────────► 退出时一并关闭
```

- **通信机制:** 本地 HTTP(方案 A)。后端在 `127.0.0.1` 监听随机端口,起轻量 HTTP server;前端用 `QNetworkAccessManager` POST `{funcId, ...}` JSON,收 JSON 响应。最贴近 jookdb 的 `postEsbWithNS` 形态,调试友好(curl 可测)。
- **进程生命周期:** 前端启动时用 `QProcess` 拉起 Java 后端 jar、传入端口;后端 ready 握手后前端开始发请求;前端退出时关闭后端子进程。对用户即"单个应用"。
- **反向通道:** Java→C++ 的回调(如进度、确认)走一条 WebSocket 或长轮询通道(对应 jookdb 的 `reversecall`)。

### 3.2 前端组件(C++/Qt,基于 jookdb 残缺源码补齐)

| 组件 | 职责 | 来源 |
|---|---|---|
| `MainWindow`(单例) | 主窗口、菜单、状态栏 | jookdb 有,复用 |
| `ContentWidget` | 中心枢纽:左对象树 + 右标签页 | jookdb 有,复用 |
| `MyTreeWidget` / `LeftWidgetForm` | 连接/库/表/视图树,双击打开 | jookdb 有 |
| `QueryForm` + `MyEdit` + `SqlLexer` | SQL 编辑器 + 结果表格 | 部分有,`MyEdit` 缺需补 |
| `TableDataForm` | 数据网格就地增删改 | 需补齐 |
| `TableForm` / `TableDesigner` | 表结构查看/设计 | 需补齐 |
| `ConnDialog` | 连接配置对话框 | 缺,需补写 |
| **`BackendClient`** ★ | 封装 HTTP,发 `{funcId,...}` 收 JSON | 新写(替代闭源 `postEsbWithNS`) |
| **`BackendProcess`** ★ | 拉起/监控/关闭 Java 子进程 | 新写 |
| `ConnData` | 连接配置数据模型 | 缺,需补写 |

★ 标记的组件是替代闭源 `utils.cpp`/`jookNS.jar` 的关键,是本项目自研价值所在。

### 3.3 后端组件(Java,从零自写)

| 组件 | 职责 |
|---|---|
| `HttpServer` | 轻量 HTTP 服务(`com.sun.net.httpserver`,零额外依赖) |
| `Dispatcher` | 按 `funcId` 路由到对应 Handler |
| `ConnectionRegistry` | 管理多个 JDBC 连接(连接池/复用) |
| `Handlers` | 执行 SQL、查元数据(库/表/列/索引)、数据增删改 |
| `ResultSetCodec` | `ResultSet` ↔ JSON 序列化 |
| `Dialect` | 方言适配:MySQL 与 SQLite 在元数据查询、分页、引号上的差异 |

**后端技术选型:** 用 JDK 自带的 `com.sun.net.httpserver`,不引入 Spring——对自用、双库场景足够,启动快、依赖少。

### 3.4 数据流(以"执行查询"为例)

1. 用户在 `QueryForm` 写 SQL,点运行。
2. `BackendClient` POST `{funcId: EXEC_SQL, connId, db, sql}`。
3. 后端 `Dispatcher` → `ExecHandler`,从 `ConnectionRegistry` 取连接执行。
4. `ResultSetCodec` 把结果转 `{columns:[...], rows:[[...]], affected, error?}`。
5. 前端渲染到结果表格;`ok=false` 时走统一错误展示。

## 4. 协议契约

统一请求/响应壳贯穿前后端:

```json
// 请求
{ "funcId": 2001, "connId": "c1", "db": "test", "sql": "select * from t" }

// 成功响应
{ "funcId": 2001, "ok": true,  "data": { "columns": [...], "rows": [...] } }

// 失败响应
{ "funcId": 2001, "ok": false, "error": { "code": "SQL_ERROR", "message": "...", "sqlState": "42S02" } }
```

前端 `BackendClient` 只判 `ok`:`true` 取 `data`,`false` 走统一错误处理。`funcId` 是一组操作码常量(如 `EXEC_SQL`、`LIST_DATABASES`、`LIST_TABLES`、`DESCRIBE_TABLE`、`UPDATE_ROW` 等),前后端共享同一份定义。

## 5. 错误处理(分 5 层兜底)

| 层级 | 失败场景 | 处理方式 |
|---|---|---|
| 进程级 | 后端没起来 / 崩溃 / 端口被占 | `BackendProcess` 监控退出码与启动握手;启动失败明确提示,运行中崩溃重启一次,再失败禁用 DB 操作并提示 |
| 通信级 | HTTP 超时 / 连不上本地端口 | `BackendClient` 统一超时,失败抛结构化错误,不卡 UI |
| 连接级 | DB 连不上 / 认证失败 / 超时 | 后端捕获,返回 `error`,前端弹提示 |
| SQL 级 | 语法错、约束冲突等 | 后端把 `SQLException` 转 `{code, sqlState, message}`,前端显示在结果区(便于对照 SQL) |
| 协议级 | 未知 funcId / 参数缺失 | 统一壳 `ok=false` + `error` 必填 |

## 6. 测试策略

- **后端(Java):** JUnit + **SQLite 内存库**做集成测试(零外部依赖、CI 可跑)。重点测 `ResultSetCodec`(类型/NULL/二进制)、`Dialect`(MySQL vs SQLite 元数据差异)、各 `Handler`。MySQL 用 Testcontainers 或本地实例做少量冒烟测试。
- **前端(C++):** Qt Test。`SqlLexer` 充分单测;`BackendClient` 对 mock HTTP server 测请求/响应/错误分支。
- **端到端:** 脚本拉起后端 + 前端,跑核心链路(建连接 → 查询 → 改一行 → 看结构),MVP 用 SQLite 文件库全自动。
- **TDD 重点:** `Dialect` 与 `ResultSetCodec` 为纯函数、边界多,先写测试再实现。

## 7. 构建与技术栈

- **前端:** C++17 + Qt5(参照 jookdb `.pro`:core/gui/widgets/network),qmake 构建。
- **后端:** Java(JDK 自带 HTTP server),Maven/Gradle 构建出可执行 jar。
- **打包:** 前端打包时附带后端 jar,启动时拉起。

## 8. 待后续迭代(非 MVP)

- 更多数据库(PostgreSQL → Oracle/达梦/SQLServer)。
- 导入导出(CSV/Excel)、数据/结构同步、用户管理。
- 连接配置持久化与加密存储。
