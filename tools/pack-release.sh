#!/usr/bin/env bash
# Portable release for Astra / Debian-like Linux (no Qt install required on client machine).
#
# Customer unpacks archive and runs:  ./sunrise
#
# Prerequisites on BUILD machine only:
#   sudo apt install build-essential cmake ninja-build \
#     qt5-qmake qtbase5-dev qtwebengine5-dev libqt5webchannel5-dev \
#     patchelf wget
#   Release build in build-astra/
#
# Usage:
#   cd ~/DokitLab/sunrise
#   mkdir -p build-astra && cd build-astra
#   cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja && ninja
#   cd ..
#   bash tools/pack-release.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-astra}"
DIST_DIR="$ROOT/dist"
APPDIR="$DIST_DIR/Sunrise.AppDir"
RELEASE_DIR="$DIST_DIR/Sunrise"
TOOLS_CACHE="$ROOT/tools/.cache"
APP_BIN="sunrise"

BINARY=""
ASSETS_SRC=""
for candidate in \
    "$BUILD_DIR/sunrise" \
    "$BUILD_DIR/Sunrise/sunrise"
do
    if [[ -f "$candidate" ]]; then
        BINARY="$candidate"
        ASSETS_SRC="$(dirname "$candidate")/assets"
        break
    fi
done

if [[ -z "$BINARY" || ! -f "$BINARY" ]]; then
    echo "ERROR: executable not found. Build first:"
    echo "  mkdir -p $BUILD_DIR && cd $BUILD_DIR"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja && ninja"
    exit 1
fi

if [[ ! -d "$ASSETS_SRC" ]]; then
    ASSETS_SRC="$ROOT/assets"
fi
if [[ ! -d "$ASSETS_SRC" ]]; then
    echo "ERROR: assets not found near binary or in project root"
    exit 1
fi

if ! command -v qmake >/dev/null 2>&1; then
    echo "ERROR: qmake not in PATH. Install: sudo apt install qt5-qmake qtbase5-dev"
    exit 1
fi

if ! command -v patchelf >/dev/null 2>&1; then
    echo "ERROR: patchelf not found. Install: sudo apt install patchelf"
    exit 1
fi

mkdir -p "$TOOLS_CACHE"
LINUXDEPLOY="${LINUXDEPLOY:-$TOOLS_CACHE/linuxdeploy-x86_64.AppImage}"
QT_PLUGIN="${QT_PLUGIN:-$TOOLS_CACHE/linuxdeploy-plugin-qt-x86_64.AppImage}"

download_if_missing() {
    local url="$1"
    local dest="$2"
    if [[ ! -x "$dest" ]]; then
        echo "Downloading $(basename "$dest") ..."
        wget -q -O "$dest" "$url"
        chmod +x "$dest"
    fi
}

download_if_missing \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    "$LINUXDEPLOY"
download_if_missing \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
    "$QT_PLUGIN"

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
cp "$BINARY" "$APPDIR/usr/bin/$APP_BIN"
chmod +x "$APPDIR/usr/bin/$APP_BIN"
cp -r "$ASSETS_SRC" "$APPDIR/usr/bin/assets"
mkdir -p "$APPDIR/usr/bin/data/scans"
mkdir -p "$APPDIR/usr/bin/key"
for extra in source.txt updates.txt aJournal.rtf; do
    if [[ -f "$ROOT/assets/$extra" ]]; then
        cp -f "$ROOT/assets/$extra" "$APPDIR/usr/bin/$extra"
    elif [[ -f "$ASSETS_SRC/$extra" ]]; then
        cp -f "$ASSETS_SRC/$extra" "$APPDIR/usr/bin/$extra"
    fi
done

ICON_SRC=""
shopt -s nullglob
for cand in \
    "$ROOT/assets/"*.ico \
    "$ASSETS_SRC/"*.ico \
    "$ROOT/assets/sysImages/"*.png \
    "$ASSETS_SRC/sysImages/"*.png
do
    ICON_SRC="$cand"
    break
done
shopt -u nullglob
if [[ -z "$ICON_SRC" ]]; then
    echo "ERROR: application icon not found in assets/"
    exit 1
fi

ICON_PNG="$APPDIR/sunrise.png"
if [[ "$ICON_SRC" == *.ico ]]; then
    if command -v convert >/dev/null 2>&1; then
        convert "$ICON_SRC" -resize 48x48 "$ICON_PNG"
    elif command -v python3 >/dev/null 2>&1; then
        python3 - "$ICON_SRC" "$ICON_PNG" <<'PY'
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert("RGBA")
im.resize((48, 48), Image.Resampling.LANCZOS).save(sys.argv[2])
PY
    else
        echo "ERROR: need ImageMagick convert or python3+Pillow to convert .ico"
        exit 1
    fi
else
    cp "$ICON_SRC" "$ICON_PNG"
fi

mkdir -p "$APPDIR/usr/share/icons/hicolor/48x48/apps"
cp "$ICON_PNG" "$APPDIR/usr/share/icons/hicolor/48x48/apps/sunrise.png"
mkdir -p "$APPDIR/usr/share/pixmaps"
cp "$ICON_PNG" "$APPDIR/usr/share/pixmaps/sunrise.png"

cat > "$APPDIR/sunrise.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Санрайс
Comment=Программа «Санрайс»
Exec=$APP_BIN
Icon=sunrise
Terminal=false
Categories=Office;Education;
EOF

echo "Bundling Qt libraries ..."
export QMAKE="$(command -v qmake)"
STUB_BIN="$TOOLS_CACHE/bin"
mkdir -p "$STUB_BIN"
cat > "$STUB_BIN/qmlimportscanner" <<'EOF'
#!/bin/sh
echo '[]'
EOF
chmod +x "$STUB_BIN/qmlimportscanner"
export PATH="$STUB_BIN:$PATH"
export QML_SOURCES_PATHS=""
"$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt --icon-file "$ICON_PNG" --output appimage >/dev/null 2>&1 || \
"$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt --icon-file "$ICON_PNG"

# Qt WebEngine helper and pak-файлы, если плагин их не положил.
QT_LIBEXECS="$("$QMAKE" -query QT_INSTALL_LIBEXECS 2>/dev/null || true)"
QT_DATA="$("$QMAKE" -query QT_INSTALL_DATA 2>/dev/null || true)"
if [[ -n "$QT_LIBEXECS" && -x "$QT_LIBEXECS/QtWebEngineProcess" ]]; then
    mkdir -p "$APPDIR/usr/libexec"
    cp -a "$QT_LIBEXECS/QtWebEngineProcess" "$APPDIR/usr/libexec/" || true
fi
if [[ -n "$QT_DATA" && -d "$QT_DATA/resources" ]]; then
    mkdir -p "$APPDIR/usr/resources"
    cp -a "$QT_DATA/resources/"*.pak "$APPDIR/usr/resources/" 2>/dev/null || true
    cp -a "$QT_DATA/resources/icudtl.dat" "$APPDIR/usr/resources/" 2>/dev/null || true
fi

rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR"

cp -a "$APPDIR/usr/bin/$APP_BIN" "$RELEASE_DIR/"
cp -a "$APPDIR/usr/bin/assets" "$RELEASE_DIR/"
for extra in source.txt updates.txt aJournal.rtf; do
    if [[ -f "$APPDIR/usr/bin/$extra" ]]; then
        cp -f "$APPDIR/usr/bin/$extra" "$RELEASE_DIR/"
    fi
done
cp -f "$ICON_PNG" "$RELEASE_DIR/sunrise.png"
mkdir -p "$RELEASE_DIR/data/scans"
mkdir -p "$RELEASE_DIR/key"
chmod 755 "$RELEASE_DIR/key"

ABS_RELEASE="$(cd "$RELEASE_DIR" && pwd)"
cat > "$RELEASE_DIR/sunrise.desktop" <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Санрайс
Comment=Программа «Санрайс»
Exec=$ABS_RELEASE/$APP_BIN
Path=$ABS_RELEASE
Icon=$ABS_RELEASE/sunrise.png
Terminal=false
Categories=Office;Education;
StartupWMClass=sunrise
EOF
chmod +x "$RELEASE_DIR/sunrise.desktop"

if command -v gio >/dev/null 2>&1; then
    gio set -t string "$RELEASE_DIR/$APP_BIN" metadata::custom-icon "file://$ABS_RELEASE/sunrise.png" 2>/dev/null || true
fi

if [[ -n "${SUDO_UID:-}" && -n "${SUDO_GID:-}" ]]; then
    chown -R "$SUDO_UID:$SUDO_GID" "$RELEASE_DIR"
fi

if [[ -d "$APPDIR/usr/lib" ]]; then
    cp -a "$APPDIR/usr/lib" "$RELEASE_DIR/"
fi
if [[ -d "$APPDIR/usr/plugins" ]]; then
    cp -a "$APPDIR/usr/plugins" "$RELEASE_DIR/"
fi
if [[ -d "$APPDIR/usr/translations" ]]; then
    cp -a "$APPDIR/usr/translations" "$RELEASE_DIR/"
fi
if [[ -d "$APPDIR/usr/libexec" ]]; then
    cp -a "$APPDIR/usr/libexec" "$RELEASE_DIR/"
fi
if [[ -d "$APPDIR/usr/resources" ]]; then
    cp -a "$APPDIR/usr/resources" "$RELEASE_DIR/"
fi
if [[ -d "$APPDIR/usr/share/qt5/resources" ]]; then
    mkdir -p "$RELEASE_DIR/resources"
    cp -a "$APPDIR/usr/share/qt5/resources/." "$RELEASE_DIR/resources/" || true
fi

cat > "$RELEASE_DIR/qt.conf" <<'EOF'
[Paths]
Prefix = .
Plugins = plugins
Translations = translations
LibraryExecutables = libexec
Data = .
EOF

echo "Patching RPATH ..."
patchelf --set-rpath '$ORIGIN/lib' "$RELEASE_DIR/$APP_BIN"
if [[ -x "$RELEASE_DIR/libexec/QtWebEngineProcess" ]]; then
    patchelf --set-rpath '$ORIGIN/../lib' "$RELEASE_DIR/libexec/QtWebEngineProcess" || true
fi

if [[ -d "$RELEASE_DIR/lib" ]]; then
    while IFS= read -r -d '' sofile; do
        patchelf --set-rpath '$ORIGIN' "$sofile" 2>/dev/null || true
    done < <(find "$RELEASE_DIR/lib" -maxdepth 1 -name '*.so*' -print0)
fi

chmod +x "$RELEASE_DIR/$APP_BIN"

echo ""
echo "Checking (must work without a wrapper script) ..."
cd "$RELEASE_DIR"
unset LD_LIBRARY_PATH QT_PLUGIN_PATH QT_QPA_PLATFORM_PLUGIN_PATH

missing="$(ldd ./$APP_BIN | grep 'not found' || true)"
if [[ -n "$missing" ]]; then
    echo "ERROR: missing libraries:"
    echo "$missing"
    ldd ./$APP_BIN
    exit 1
fi

echo ""
echo "Release ready: $RELEASE_DIR"
echo ""
echo "Send to customer:"
echo "  tar -czvf Sunrise.tar.gz -C dist Sunrise"
echo ""
echo "Customer runs (from unpacked folder):"
echo "  chmod +x sunrise sunrise.desktop"
echo "  ./sunrise"
