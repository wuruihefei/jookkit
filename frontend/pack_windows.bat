@echo off
REM ============================================================
REM  JookKit Windows 打包脚本
REM  在 "Qt 5.15 (MinGW) 命令行" 中运行(确保 qmake / mingw32-make /
REM  windeployqt 在 PATH 中),并已安装 Maven 与 JDK 17。
REM  产物: dist\JookKit-win.zip
REM ============================================================
setlocal enabledelayedexpansion
cd /d %~dp0

echo [1/6] 检查工具链...
where qmake >nul 2>nul || (echo 缺少 qmake,请在 Qt MinGW 命令行运行 & exit /b 1)
where mingw32-make >nul 2>nul || (echo 缺少 mingw32-make & exit /b 1)
where windeployqt >nul 2>nul || (echo 缺少 windeployqt & exit /b 1)
where mvn >nul 2>nul || (echo 缺少 mvn ^(Maven^) & exit /b 1)
where java >nul 2>nul || (echo 缺少 java ^(JDK17^) & exit /b 1)

echo [2/6] 构建后端 jar...
pushd ..\backend
call mvn -q package -DskipTests || (echo 后端构建失败 & popd & exit /b 1)
popd

echo [3/6] 构建前端 release...
if exist build-win rmdir /s /q build-win
mkdir build-win
cd build-win
qmake ..\jookkit.pro "CONFIG+=release" || (echo qmake 失败 & cd .. & exit /b 1)
mingw32-make -j4 || (echo 编译失败 & cd .. & exit /b 1)
cd ..

echo [4/6] 组装 dist...
set DIST=dist\JookKit
if exist dist rmdir /s /q dist
mkdir %DIST%
mkdir %DIST%\backend

REM release 产物可能在 build-win\release\ 或 build-win\
if exist build-win\release\jookkit.exe (
    copy /y build-win\release\jookkit.exe %DIST%\ >nul
) else (
    copy /y build-win\jookkit.exe %DIST%\ >nul
)
copy /y ..\backend\target\jookkit-backend.jar %DIST%\backend\ >nul

echo [5/6] windeployqt 收集 Qt 依赖 DLL...
windeployqt --release --no-translations %DIST%\jookkit.exe || (echo windeployqt 失败 & exit /b 1)

echo [6/6] 打 zip...
powershell -NoProfile -Command "Compress-Archive -Path 'dist\JookKit\*' -DestinationPath 'dist\JookKit-win.zip' -Force"

echo.
echo 完成: dist\JookKit-win.zip
echo 运行: 解压后双击 jookkit.exe(目标机需安装 Java 17)
echo       jar 已放在 jookkit.exe 旁的 backend\ 目录,程序会自动找到。
endlocal
