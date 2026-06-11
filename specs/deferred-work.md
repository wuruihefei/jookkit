# Deferred Work

- 2026-06-11 文件菜单"打开 SQL 文件..."与工具菜单"执行 SQL 文件..."指向同一处理函数 `MainWindow::executeSqlFile`,两处文案不一致("打开" vs "执行"),存在轻微 UX 误导。历史遗留,与工具菜单隐藏改动无关。
