# JookKit

一个轻量的桌面数据库管理工具：**C++/Qt5 前端 + Java 后端（JDBC）双进程架构**，两者通过本地 HTTP + JSON 协议通信。支持 MySQL / MariaDB / SQLite。

## 项目背景

JookKit 在架构上复刻了 jookdb 的实际实现路径——C++/Qt 负责全部 UI，Java 子进程负责全部数据库访问（JDBC），前端启动时拉起后端、退出时一并关闭，对用户而言就是一个单体应用。

之所以重新开发而非直接使用 jookdb：

- jookdb 公开的 C++ 前端源码为 **GPLv3**，但**残缺**（缺少 `utils.h`、`globalutils.cpp`、`conndialog.cpp` 等核心文件），无法直接编译；
- jookdb 的后端 `jookNS.jar` 是**闭源二进制**，含授权逻辑，既不能复用也不能合法分发。

因此 JookKit 的路径是：**前端基于 jookdb 的残缺 GPLv3 源码补齐，后端从零自写**，并且完全去除授权/试用/激活等机制。

```
┌─────────────────────────────┐         ┌──────────────────────────────┐
│   前端 (C++ / Qt5)          │  HTTP   │   后端 (Java)                │
│   主进程，负责全部 UI       │ ──────► │   子进程，负责全部 DB 访问   │
│   QNetworkAccessManager     │ POST    │   127.0.0.1:<随机端口>       │
│                             │ {funcId}│                              │
│                             │ ◄────── │   JDBC → MySQL/MariaDB/SQLite│
└─────────────────────────────┘  JSON   └──────────────────────────────┘
      启动时 QProcess 拉起后端 ────────────► 退出时一并关闭
```

## 主要功能

- **连接管理**：新建/编辑/测试 MySQL、MariaDB、SQLite 连接，连接配置本地持久化
- **对象树浏览**：连接 → 数据库 → 表/视图层级浏览，双击打开
- **SQL 编辑器**：语法高亮、SQL 补全、SQL 格式化、多语句执行、查找替换
- **查询结果**：结果网格展示，查询页顶部可切换目标连接与数据库
- **表数据编辑**：数据网格就地增/删/改，分页浏览（默认每页 20 条）
- **表结构查看与编辑**：字段改名/改类型/可空/增删列，类型下拉随数据源（MySQL/SQLite），本地校验后生成 ALTER 确认执行
- **用户管理**（MySQL）：用户的查看与管理
- **信息窗格**：DDL、索引等对象信息展示

## 构建与打包

### 前置依赖

- 后端：JDK 17、Maven
- 前端：Qt 5.15（core/gui/widgets/network/testlib）、qmake、g++、make

### 后端（Java）

```bash
cd backend
mvn package        # 产物: target/jookkit-backend.jar (fat jar, 含全部依赖)
mvn test           # 运行单元/集成测试
```

### 前端（Linux）

```bash
cd frontend
qmake jookkit.pro && make
./jookkit          # 默认在 ../backend/target/ 查找后端 jar
# 或显式指定: JOOKKIT_JAR=/abs/path/jookkit-backend.jar ./jookkit
```

测试：

```bash
qmake test.pro && make
QT_QPA_PLATFORM=offscreen ./tst_jookkit          # 单元测试（离屏）
qmake test_integration.pro && make
JOOKKIT_JAR=../backend/target/jookkit-backend.jar QT_QPA_PLATFORM=offscreen ./tst_integration
```

### Windows 打包

两种方式（详见 [frontend/WINDOWS.md](frontend/WINDOWS.md)）：

1. **Windows 原生构建**：在 Qt 5.15 (MinGW 64-bit) 命令行中运行 `frontend\pack_windows.bat`，自动完成「后端 jar 构建 → qmake/mingw32-make 编 release → windeployqt 收集 DLL → 打 zip」，产物 `frontend\dist\JookKit-win.zip`（目标机需安装 Java 17）。
2. **Linux 交叉编译**：基于 MXE Qt5 的 Docker 交叉编译，运行 `frontend/docker/pack-cross.sh`，产出自包含 zip（exe + Qt DLL + 内置 JRE + 后端 jar），目标机免装 Java。

## 开源协议

本项目以 **GNU General Public License v3.0 (GPL-3.0)** 发布。

说明：前端部分基于 jookdb 公开的 GPLv3 C++ 源码补齐而来，属于 GPLv3 衍生作品，依 copyleft 要求整体沿用 GPL-3.0；后端为从零自写代码，随项目一并以 GPL-3.0 发布。

## 免责声明

- 本软件按"现状"（AS IS）提供，**不附带任何明示或暗示的担保**，包括但不限于适销性、特定用途适用性的担保（详见 GPL-3.0 第 15、16 条）。
- 本软件可直接对数据库执行查询、修改、删除等操作。**请在操作生产数据前自行备份**，因使用本软件造成的任何数据丢失、损坏或其他损失，作者不承担责任。
- 本软件仅供学习与日常数据库管理使用，请勿用于任何违反法律法规或所在组织规定的用途。
- 本项目与 jookdb 官方无任何隶属或合作关系。

## 致谢

特别感谢 **【提前就餐群】** 的各位好友对本项目的"嘲讽" :)
——正是这些宝贵的冷嘲热讽，持续为开发提供了不竭的动力。

## 依赖的开源项目

| 项目 | 用途 | 协议 |
|---|---|---|
| [Qt 5](https://www.qt.io/) | 前端 GUI 框架（core/gui/widgets/network） | LGPL-3.0 / GPL |
| [jookdb 前端源码](https://jookdb.com/) | 前端 UI 的基础（残缺源码补齐） | GPL-3.0 |
| [Gson](https://github.com/google/gson) | 后端 JSON 序列化 | Apache-2.0 |
| [SQLite JDBC (xerial)](https://github.com/xerial/sqlite-jdbc) | SQLite 驱动 | Apache-2.0 |
| [MariaDB Connector/J](https://github.com/mariadb-corporation/mariadb-connector-j) | MariaDB/MySQL 驱动 | LGPL-2.1 |
| [MySQL Connector/J](https://github.com/mysql/mysql-connector-j) | MySQL 8 驱动 | GPL-2.0（含 Universal FOSS Exception） |
| [JUnit 5](https://junit.org/junit5/) | 后端测试框架（仅测试期） | EPL-2.0 |
| [MXE](https://mxe.cc/) | Linux→Windows 交叉编译工具链（仅构建期） | MIT |
