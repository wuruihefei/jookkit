# JookKit Backend

JookKit 的 Java 后端:本地 HTTP + JSON,通过 JDBC 操作 MySQL/MariaDB/SQLite。

## 构建

    mvn package          # 产物:target/jookkit-backend.jar(fat jar,含依赖与 Main-Class)
    mvn test             # 运行全部单元/集成测试

## 运行

    java -jar target/jookkit-backend.jar --port <端口>
    # 省略 --port 则随机分配;实际端口打印为 stdout 的 JOOKKIT_PORT=<n>(供前端握手)

## 协议

POST `/rpc`,请求/响应壳见 `../docs/superpowers/specs/2026-06-10-jookkit-design.md` 第 4 节。

操作码(funcId):
- 1001 TEST_CONNECTION / 1002 OPEN_CONNECTION / 1003 CLOSE_CONNECTION
- 2001 LIST_DATABASES / 2002 LIST_TABLES / 2003 DESCRIBE_TABLE
- 3001 EXEC_SQL
- 4001 INSERT_ROW / 4002 UPDATE_ROW / 4003 DELETE_ROW

示例:

    curl -s -X POST http://127.0.0.1:8765/rpc -H 'Content-Type: application/json' \
      -d '{"funcId":1002,"connId":"c1","type":"sqlite","file":":memory:"}'
    curl -s -X POST http://127.0.0.1:8765/rpc -H 'Content-Type: application/json' \
      -d '{"funcId":3001,"connId":"c1","sql":"select 42 as answer"}'

## 构建工具说明

原计划用 Gradle,因环境限制改用 Maven(见
`../docs/superpowers/plans/2026-06-10-backend.md` 顶部适配说明)。Java 源码一致。
