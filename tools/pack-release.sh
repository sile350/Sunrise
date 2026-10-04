#!/usr/bin/env bash
# Портативный дистрибутив Санрайс для Astra / Debian-like Linux.
# Клиенту Qt ставить не нужно: рядом с ./sunrise лежат lib/, plugins/, qt.conf.
#
# На машине сборки:
#   sudo apt install build-essential cmake ninja-build \
#     qt5-qmake qtbase5-dev libqt5webkit5-dev patchelf wget
#   (опционально: imagemagick или python3-pil — для иконки из .ico)
#
#   cd ~/Sunrise/Sunrise
#   mkdir -p build-astra && cd build-astra
#   cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja && ninja
#   cd ..
#   bash tools/pack-release.sh
#
# Результат: dist/Sunrise/  и  dist/Sunrise.tar.gz
# Повторная упаковка не затирает dist/Sunrise/data и dist/Sunrise/key.

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
    echo "ERROR: не найден $BUILD_DIR/sunrise. Сначала соберите:"
    echo "  mkdir -p $BUILD_DIR && cd $BUILD_DIR"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja && ninja"
    exit 1
fi

if [[ ! -d "$ASSETS_SRC" ]]; then
    ASSETS_SRC="$ROOT/assets"
fi
if [[ ! -d "$ASSETS_SRC" ]]; then
    echo "ERROR: не найден каталог assets"
    exit 1
fi

if ! command -v qmake >/dev/null 2>&1; then
    echo "ERROR: qmake не в PATH. sudo apt install qt5-qmake qtbase5-dev"
    exit 1
fi
if ! command -v patchelf >/dev/null 2>&1; then
    echo "ERROR: patchelf не найден. sudo apt install patchelf"
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

# На Astra FUSE для AppImage часто выключен.
export APPIMAGE_EXTRACT_AND_RUN=1

run_appimage() {
    local appimage="$1"
    shift
    if "$appimage" --appimage-extract-and-run --help >/dev/null 2>&1 \
        || "$appimage" --appimage-help >/dev/null 2>&1; then
        "$appimage" --appimage-extract-and-run "$@"
    else
        "$appimage" "$@"
    fi
}

make_icon_png() {
    local src="$1"
    local dest="$2"
    mkdir -p "$(dirname "$dest")"
    rm -f "$dest"
    if [[ "$src" == *.png ]]; then
        cp -f "$src" "$dest"
        return 0
    fi
    local tmpdir
    tmpdir="$(mktemp -d)"
    local out="$tmpdir/icon.png"
    if command -v convert >/dev/null 2>&1; then
        # Многокадровый .ico иначе даёт icon-0.png, icon-1.png и не создаёт dest.
        convert "${src}[0]" -resize 48x48 "$out" 2>/dev/null \
            || convert "$src" -resize 48x48 -flatten "$out" 2>/dev/null \
            || true
        if [[ ! -f "$out" ]]; then
            shopt -s nullglob
            local generated=("$tmpdir"/icon*.png)
            shopt -u nullglob
            if [[ ${#generated[@]} -gt 0 ]]; then
                cp -f "${generated[0]}" "$out"
            fi
        fi
    fi
    if [[ ! -f "$out" ]] && command -v python3 >/dev/null 2>&1; then
        python3 - "$src" "$out" <<'PY' || true
import sys
src, dest = sys.argv[1], sys.argv[2]
try:
    from PIL import Image
    im = Image.open(src)
    im = im.convert("RGBA")
    im = im.resize((48, 48), getattr(Image, "Resampling", Image).LANCZOS if hasattr(Image, "LANCZOS") else Image.BICUBIC)
    im.save(dest)
except Exception:
    sys.exit(1)
PY
    fi
    if [[ -f "$out" ]]; then
        cp -f "$out" "$dest"
        rm -rf "$tmpdir"
        return 0
    fi
    rm -rf "$tmpdir"
    return 1
}

ICON_SRC=""
shopt -s nullglob
for cand in \
    "$ROOT/assets/"*.ico \
    "$ASSETS_SRC/"*.ico \
    "$ROOT/assets/sysImages/logo2.png" \
    "$ASSETS_SRC/sysImages/logo2.png" \
    "$ROOT/assets/sysImages/about.png" \
    "$ASSETS_SRC/sysImages/about.png"
do
    if [[ -f "$cand" ]]; then
        ICON_SRC="$cand"
        break
    fi
done
shopt -u nullglob
if [[ -z "$ICON_SRC" ]]; then
    echo "ERROR: не найдена иконка (assets/*.ico или sysImages/logo2.png)"
    exit 1
fi

ICON_PNG="$TOOLS_CACHE/sunrise.png"
echo "Icon source: $ICON_SRC"
if ! make_icon_png "$ICON_SRC" "$ICON_PNG"; then
    echo "WARNING: не удалось конвертировать .ico (поставьте imagemagick или python3-pil)."
    echo "         Беру запасную PNG."
    if [[ -f "$ASSETS_SRC/sysImages/logo2.png" ]]; then
        cp -f "$ASSETS_SRC/sysImages/logo2.png" "$ICON_PNG"
    elif [[ -f "$ROOT/assets/sysImages/logo2.png" ]]; then
        cp -f "$ROOT/assets/sysImages/logo2.png" "$ICON_PNG"
    else
        echo "ERROR: нет запасной PNG для ярлыка"
        exit 1
    fi
fi
if [[ ! -f "$ICON_PNG" ]]; then
    echo "ERROR: не создан $ICON_PNG"
    exit 1
fi

# Сохраняем data/key при повторной упаковке.
BACKUP_DATA=""
BACKUP_KEY=""
if [[ -d "$RELEASE_DIR/data" ]]; then
    BACKUP_DATA="$(mktemp -d)"
    cp -a "$RELEASE_DIR/data/." "$BACKUP_DATA/"
fi
if [[ -d "$RELEASE_DIR/key" ]]; then
    BACKUP_KEY="$(mktemp -d)"
    cp -a "$RELEASE_DIR/key/." "$BACKUP_KEY/"
fi

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
cp "$BINARY" "$APPDIR/usr/bin/$APP_BIN"
chmod +x "$APPDIR/usr/bin/$APP_BIN"
cp -a "$ASSETS_SRC" "$APPDIR/usr/bin/assets"
mkdir -p "$APPDIR/usr/bin/data/scans"
mkdir -p "$APPDIR/usr/bin/key"
for extra in source.txt updates.txt aJournal.rtf; do
    if [[ -f "$ROOT/assets/$extra" ]]; then
        cp -f "$ROOT/assets/$extra" "$APPDIR/usr/bin/$extra"
    elif [[ -f "$ASSETS_SRC/$extra" ]]; then
        cp -f "$ASSETS_SRC/$extra" "$APPDIR/usr/bin/$extra"
    fi
done

cp -f "$ICON_PNG" "$APPDIR/sunrise.png"
mkdir -p "$APPDIR/usr/share/icons/hicolor/48x48/apps"
cp -f "$ICON_PNG" "$APPDIR/usr/share/icons/hicolor/48x48/apps/sunrise.png"
mkdir -p "$APPDIR/usr/share/pixmaps"
cp -f "$ICON_PNG" "$APPDIR/usr/share/pixmaps/sunrise.png"

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
export QMAKE
QMAKE="$(command -v qmake)"
STUB_BIN="$TOOLS_CACHE/bin"
mkdir -p "$STUB_BIN"
cat > "$STUB_BIN/qmlimportscanner" <<'EOF'
#!/bin/sh
echo '[]'
EOF
chmod +x "$STUB_BIN/qmlimportscanner"
export PATH="$STUB_BIN:$PATH"
export QML_SOURCES_PATHS=""

set +e
run_appimage "$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt --icon-file "$ICON_PNG"
deploy_rc=$?
set -e
if [[ $deploy_rc -ne 0 ]]; then
    echo "WARNING: linuxdeploy вернул код $deploy_rc — пробую ещё раз без plugin-ошибок."
    set +e
    run_appimage "$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt --icon-file "$ICON_PNG"
    set -e
fi

if [[ ! -x "$APPDIR/usr/bin/$APP_BIN" ]]; then
    echo "ERROR: linuxdeploy не оставил $APPDIR/usr/bin/$APP_BIN"
    exit 1
fi

# WebEngine extras, если бинарник всё же собран с WebEngine.
QT_LIBEXECS="$("$QMAKE" -query QT_INSTALL_LIBEXECS 2>/dev/null || true)"
QT_DATA="$("$QMAKE" -query QT_INSTALL_DATA 2>/dev/null || true)"
QT_PLUGINS="$("$QMAKE" -query QT_INSTALL_PLUGINS 2>/dev/null || true)"
QT_LIBS="$("$QMAKE" -query QT_INSTALL_LIBS 2>/dev/null || true)"
if [[ -n "$QT_LIBEXECS" && -x "$QT_LIBEXECS/QtWebEngineProcess" ]]; then
    mkdir -p "$APPDIR/usr/libexec"
    cp -a "$QT_LIBEXECS/QtWebEngineProcess" "$APPDIR/usr/libexec/" || true
fi
if [[ -n "$QT_DATA" && -d "$QT_DATA/resources" ]]; then
    mkdir -p "$APPDIR/usr/resources"
    cp -a "$QT_DATA/resources/." "$APPDIR/usr/resources/" 2>/dev/null || true
fi

copy_plugin_dir() {
    local name="$1"
    if [[ -n "$QT_PLUGINS" && -d "$QT_PLUGINS/$name" ]]; then
        mkdir -p "$APPDIR/usr/plugins/$name"
        cp -a "$QT_PLUGINS/$name/." "$APPDIR/usr/plugins/$name/" 2>/dev/null || true
    fi
}
copy_plugin_dir platforms
copy_plugin_dir imageformats
copy_plugin_dir sqldrivers
copy_plugin_dir printsupport
copy_plugin_dir xcbglintegrations
copy_plugin_dir platforminputcontexts

# Дотянуть .so, которые linuxdeploy мог пропустить (WebKit).
if [[ -n "$QT_LIBS" && -d "$APPDIR/usr/lib" ]]; then
    while IFS= read -r dep; do
        base="$(basename "$dep")"
        if [[ ! -e "$APPDIR/usr/lib/$base" && -f "$dep" ]]; then
            cp -a "$dep" "$APPDIR/usr/lib/" || true
        fi
    done < <(ldd "$APPDIR/usr/bin/$APP_BIN" | awk '/=> \// {print $3}' | grep -E 'libQt5|libpng|libjpeg|libssl|libcrypto|libicu|libwebp|libxslt|libxml|libsqlite' || true)
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

if [[ -n "$BACKUP_DATA" ]]; then
    cp -a "$BACKUP_DATA/." "$RELEASE_DIR/data/"
    rm -rf "$BACKUP_DATA"
fi
if [[ -n "$BACKUP_KEY" ]]; then
    cp -a "$BACKUP_KEY/." "$RELEASE_DIR/key/"
    rm -rf "$BACKUP_KEY"
fi

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
chmod +x "$RELEASE_DIR/sunrise.desktop" "$RELEASE_DIR/$APP_BIN"

if command -v gio >/dev/null 2>&1; then
    gio set -t string "$RELEASE_DIR/$APP_BIN" metadata::custom-icon "file://$ABS_RELEASE/sunrise.png" 2>/dev/null || true
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
patchelf --set-rpath '$ORIGIN/lib' "$RELEASE_DIR/$APP_BIN" || true
if [[ -x "$RELEASE_DIR/libexec/QtWebEngineProcess" ]]; then
    patchelf --set-rpath '$ORIGIN/../lib' "$RELEASE_DIR/libexec/QtWebEngineProcess" || true
fi
if [[ -d "$RELEASE_DIR/lib" ]]; then
    while IFS= read -r -d '' sofile; do
        patchelf --set-rpath '$ORIGIN' "$sofile" 2>/dev/null || true
    done < <(find "$RELEASE_DIR/lib" -maxdepth 1 -name '*.so*' -print0)
fi

if [[ -n "${SUDO_UID:-}" && -n "${SUDO_GID:-}" ]]; then
    chown -R "$SUDO_UID:$SUDO_GID" "$RELEASE_DIR"
fi

echo ""
echo "Checking libraries ..."
cd "$RELEASE_DIR"
unset LD_LIBRARY_PATH QT_PLUGIN_PATH QT_QPA_PLATFORM_PLUGIN_PATH || true
missing="$(ldd ./$APP_BIN | grep 'not found' || true)"
if [[ -n "$missing" ]]; then
    echo "WARNING: часть библиотек не найдена (на клиенте они могут быть системными):"
    echo "$missing"
fi

echo "Creating dist/Sunrise.tar.gz ..."
mkdir -p "$DIST_DIR"
tar -C "$DIST_DIR" -czf "$DIST_DIR/Sunrise.tar.gz" Sunrise

echo ""
echo "OK: $RELEASE_DIR"
echo "ZIP/TAR: $DIST_DIR/Sunrise.tar.gz"
echo ""
echo "Клиент:"
echo "  tar -xzf Sunrise.tar.gz"
echo "  cd Sunrise"
echo "  chmod +x sunrise sunrise.desktop"
echo "  ./sunrise"
