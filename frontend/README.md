# JookKit Frontend

JookKit 的 C++/Qt5 前端:启动时拉起 Java 后端子进程,经本地 HTTP + JSON 通信。

## 依赖

- Qt 5(core/gui/widgets/network/testlib)、qmake、g++、make
- 运行期需要后端 fat jar(见 `../backend/`,`mvn package` 产出 `backend/target/jookkit-backend.jar`)

## 构建与运行

    qmake jookkit.pro && make
    # jar 路径:默认取 <可执行目录>/../backend/target/jookkit-backend.jar
    # 或用环境变量覆盖:
    JOOKKIT_JAR=/abs/path/to/jookkit-backend.jar ./jookkit

最小窗口(MinimalWindow)功能:工具栏"打开 SQLite"选库 → 左侧列表显示表 → 编辑器写 SQL →
"运行 SQL" → 结果表格。需要图形显示(WSL 用 WSLg)。

## 测试

单元测试(纯逻辑,无需后端/显示,可离屏):

    qmake test.pro && make
    QT_QPA_PLATFORM=offscreen ./tst_jookkit

集成测试(真实拉起后端 jar,跑通连接/查询/错误回传):

    qmake test_integration.pro && make
    JOOKKIT_JAR=../backend/target/jookkit-backend.jar QT_QPA_PLATFORM=offscreen ./tst_integration

## 组件

- `backend/ConnData` — 连接配置模型,`toOpenRequest()` 产出 OPEN_CONNECTION 请求
- `backend/BackendClient` — 封装 QNetworkAccessManager,阻塞式 `call()` 发 JSON 收 `{ok,data,error}`
- `backend/BackendProcess` — QProcess 拉起 jar,读 stdout `JOOKKIT_PORT=<n>` 握手,析构时关子进程
- `backend/FuncId` — 与后端一一对应的操作码常量
- `ui/MinimalWindow` — 最小可用窗口(Phase A 成果;Phase B 将以 jookdb 移植的完整 UI 替换)
