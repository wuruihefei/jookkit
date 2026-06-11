---
title: '表结构字段编辑:修改/校验/类型下拉框随数据源'
type: 'feature'
created: '2026-06-11'
status: 'done'
route: 'plan-code-review'
---

# 表结构字段编辑

## Intent

**Problem:** 结构标签页只读,无法修改表字段;用户要求支持字段修改与校验,字段类型用下拉框编辑,且下拉选项须与当前数据源(MySQL/SQLite)匹配。

**Approach:** 改造 `TableStructureForm` 为可编辑:列名/可空可直接编辑,类型列用可编辑 QComboBox(选项按连接类型给 MySQL 或 SQLite 类型表);工具栏提供 新增字段/删除字段/保存修改/刷新。保存时本地校验(标识符合法性、重名、类型与数据源匹配、SQLite 不支持改类型/可空等限制),生成 ALTER TABLE 语句,确认后经 EXEC_SQL 逐条执行并刷新。

## Tasks

1. `frontend/src/ui/TableStructureForm.h/.cpp` — 可编辑网格 + 类型下拉 + 校验 + ALTER 生成/执行。
2. `frontend/src/ui/ContentWidget.cpp` — openTableStructure 传入连接类型(mysql/sqlite)。

## ACs

- Given MySQL 连接, When 打开结构页编辑类型下拉, Then 选项为 MySQL 类型(INT/VARCHAR(255)/DATETIME 等),可手输带长度类型。
- Given SQLite 连接, When 编辑类型下拉, Then 选项仅 INTEGER/TEXT/REAL/NUMERIC/BLOB。
- Given 非法列名/重名/类型与数据源不匹配, When 保存, Then 弹出校验错误,不执行 SQL。
- Given SQLite 下修改已有列类型或可空性, When 保存, Then 校验报"SQLite 不支持",不执行。
- Given 合法修改(MySQL 改名/改类型/改可空;SQLite 改名/增删列), When 保存并确认, Then 生成的 ALTER 语句执行成功且结构刷新。
