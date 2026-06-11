#!/bin/bash
# 在 Linux 上一键交叉编译并打包 JookKit 的 Windows(x64)自包含 zip。
# 依赖:docker(可访问)、curl、unzip、zip、mvn、网络(经 HTTP(S)_PROXY)。
# 产物:frontend/dist-win/JookKit-win.zip(含 exe + Qt DLL + 内置JRE + 后端jar)。
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)     # frontend/docker
FE=$(cd "$HERE/.." && pwd)              # frontend
ROOT=$(cd "$FE/.." && pwd)             # repo root
OUT="$FE/dist-win"
PKG="$OUT/JookKit"
JRE_URL="https://api.adoptium.net/v3/binary/latest/17/ga/windows/x64/jre/hotspot/normal/eclipse"

echo "[1/7] 准备容器 TLS 用 CA 包"
cp /etc/ssl/certs/ca-certificates.crt "$HERE/ca-certificates.crt"

echo "[2/7] 构建交叉编译镜像(MXE Qt5)"
docker build --build-arg http_proxy="${HTTP_PROXY:-}" --build-arg https_proxy="${HTTPS_PROXY:-}" \
  -t jookkit-cross -f "$HERE/Dockerfile.cross" "$HERE"

echo "[3/7] 构建后端 fat jar"
( cd "$ROOT/backend" && mvn -q package -DskipTests )

echo "[4/7] 交叉编译 exe + 打包依赖 DLL"
mkdir -p "$OUT"
docker run --rm -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" \
  -e http_proxy="${HTTP_PROXY:-}" -e https_proxy="${HTTPS_PROXY:-}" \
  -v "$FE":/work:ro -v "$OUT":/out jookkit-cross bash /work/docker/build.sh

echo "[5/7] 内置 Windows JRE17(缓存于 docker/jre-win.zip)"
[ -f "$HERE/jre-win.zip" ] || curl -sL -o "$HERE/jre-win.zip" "$JRE_URL"
rm -rf "$OUT/jreroot" && mkdir -p "$OUT/jreroot"
unzip -q "$HERE/jre-win.zip" -d "$OUT/jreroot"
JRE_DIR=$(ls -d "$OUT"/jreroot/*/ | head -1)
rm -rf "$PKG/jre" && cp -r "$JRE_DIR" "$PKG/jre" && rm -rf "$OUT/jreroot"

echo "[6/7] 放入后端 jar + 使用说明"
mkdir -p "$PKG/backend"
cp "$ROOT/backend/target/jookkit-backend.jar" "$PKG/backend/"
cat > "$PKG/使用说明.txt" <<'EOF'
JookKit (Windows x64) —— 双击 jookkit.exe 即可,无需安装 Java(已内置 jre\)。
新建连接:SQLite 选文件;MySQL 填连接信息,"参数"默认
useSSL=false&allowPublicKeyRetrieval=true(MySQL8 认证需要),
需要时区再加 &serverTimezone=GMT%2B8。
仅供测试验证。
EOF

echo "[7/7] 打 zip"
( cd "$OUT" && rm -f JookKit-win.zip && zip -r -q JookKit-win.zip JookKit )
echo "完成: $OUT/JookKit-win.zip"
