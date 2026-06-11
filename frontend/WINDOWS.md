# 在 Windows 上构建与打包 JookKit

> 前端是 Qt C++ 应用,**Windows 可执行文件必须在 Windows + Qt 环境构建**
> (无法从 Linux 直接产出)。后端 jar 跨平台,目标机只需装 Java 17。

## 前置

- Windows 10/11
- Qt **5.15** + **MinGW**(随 Qt 安装的那套);把 `Qt\5.15.x\mingw81_64\bin` 和
  MinGW 的 `bin` 加入 `PATH`(使 `qmake`/`mingw32-make`/`windeployqt` 可用)
- **Maven** 与 **JDK 17**(`mvn`/`java` 在 PATH)

## 一键打包

打开 "Qt 5.15 (MinGW 64-bit)" 命令行,进入 `frontend\` 目录,运行:

    pack_windows.bat

产物:`frontend\dist\JookKit-win.zip`。脚本会:
1. 用 Maven 构建后端 fat jar
2. 用 qmake+mingw32-make 编 release 版 `jookkit.exe`
3. `windeployqt` 收集 Qt 运行所需 DLL
4. 把后端 jar 放到 `jookkit.exe` 旁的 `backend\` 目录
5. 压成 zip

## 运行

解压 zip,双击 `jookkit.exe`。程序按顺序查找后端 jar:
1. 环境变量 `JOOKKIT_JAR`
2. exe 同目录的 `jookkit-backend.jar`
3. exe 同目录下的 `backend\jookkit-backend.jar`(打包默认)
4. 开发布局 `..\backend\target\jookkit-backend.jar`

**目标机需安装 Java 17**(`java` 在 PATH)。后续可用 jlink 打包精简 JRE 一起分发,免装 Java(未做)。

## 连接 MySQL 注意

新建 MySQL 连接时,"参数" 一栏默认 `useSSL=false&allowPublicKeyRetrieval=true`
(MySQL 8 默认 caching_sha2 over 明文需要)。如服务器要求时区,可加
`&serverTimezone=GMT%2B8`。
