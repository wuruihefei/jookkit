---
title: '工具菜单隐藏未实现功能'
type: 'chore'
created: '2026-06-11'
status: 'done'
route: 'one-shot'
---

# 工具菜单隐藏未实现功能

## Intent

**Problem:** 工具菜单中"数据传输/数据同步/结构同步"三项为永久置灰的占位项，用户看到却不能用，造成困扰。

**Approach:** 直接移除这三个置灰项及其后的分隔符，菜单留注释说明未实现功能暂不展示；`disabled` 占位 lambda 仍被收藏/帮助菜单使用，保持不动。

## Suggested Review Order

1. [MainWindow.cpp — buildMenus 工具菜单段](../frontend/src/ui/MainWindow.cpp) — 唯一改动点：删 3 个置灰项 + 1 个分隔符。
