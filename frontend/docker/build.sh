#!/bin/bash
# 在 jookkit-cross 容器内:交叉编译 jookkit.exe 并打包依赖 DLL 到 /out/JookKit。
set -euo pipefail

QMAKE=$(ls /usr/lib/mxe/usr/bin/*qmake-qt5 | head -1)
OBJDUMP=$(ls /usr/lib/mxe/usr/bin/*-objdump | head -1)
TGT=/usr/lib/mxe/usr/x86_64-w64-mingw32.shared
SEARCH="$TGT/bin $TGT/qt5/bin"

rm -rf /tmp/b && mkdir -p /tmp/b
cp -r /work/src /work/jookkit.pro /tmp/b/
cd /tmp/b
"$QMAKE" jookkit.pro >/dev/null
make -j4 >/dev/null 2>&1

DEST=/out/JookKit
rm -rf "$DEST" && mkdir -p "$DEST/platforms"
cp /tmp/b/release/jookkit.exe "$DEST/"
cp "$TGT/qt5/plugins/platforms/qwindows.dll" "$DEST/platforms/"

find_dll() {
  local n=$1 d
  for d in $SEARCH; do
    if [ -f "$d/$n" ]; then echo "$d/$n"; return 0; fi
  done
  return 1
}

declare -A seen
resolve() {
  local f=$1 dll src
  for dll in $("$OBJDUMP" -p "$f" 2>/dev/null | awk '/DLL Name:/{print $3}'); do
    if [ -n "${seen[$dll]:-}" ]; then continue; fi
    seen[$dll]=1
    if src=$(find_dll "$dll"); then
      cp "$src" "$DEST/"
      resolve "$DEST/$dll"
    fi
  done
}
resolve "$DEST/jookkit.exe"
resolve "$DEST/platforms/qwindows.dll"

chown -R "${HOST_UID}:${HOST_GID}" /out

echo "RESULT_DLL_COUNT=$(ls "$DEST"/*.dll | wc -l)"
echo "RESULT_HAS_QT=$(ls "$DEST" | grep -ci qt5)"
ls "$DEST"
