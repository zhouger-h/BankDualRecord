#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════
# 银行柜面双录控件 - 一键构建 + 打包 .deb（修复版）
# 关键修复：移除 systemd GUI 启动，改为 autostart + xdg-open
# ═══════════════════════════════════════════════════════════════

set -e

APP_NAME="dualrecord"
APP_DISPLAY="银行柜面双录控件"
APP_VERSION="1.0.0"
APP_ARCH="amd64"
MAINTAINER="SiroInfo support@siro-info.com"
DESCRIPTION="银行柜面双录控件 - 基于Qt5/GStreamer的双路音视频录制服务"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

OUT_DIR="${PROJECT_ROOT}/out"
BUILD_BIN="${OUT_DIR}/linux/DualRecordService"
RELEASE_DIR="${PROJECT_ROOT}/release"

DEB_ROOT="${OUT_DIR}/deb_build"
PKG_DIR="${DEB_ROOT}/${APP_NAME}_${APP_VERSION}_${APP_ARCH}"
INSTALL_DIR="${PKG_DIR}/opt/dualrecord"

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'; NC='\033[0m'
info() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
err()  { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

cd "${PROJECT_ROOT}"

rm -rf "${RELEASE_DIR}"

echo "=========================================="
echo "  双录控件 - 构建 + 打包（修复版）"
echo "=========================================="

# ─────────────────────────────────────────
# Step 1: Qt 检查
# ─────────────────────────────────────────
info "[1/6] 检查 Qt..."

if ! command -v qmake > /dev/null 2>&1; then
    warn "未找到 qmake，安装依赖..."
    apt-get update
    apt-get install -y \
        qtbase5-dev qtchooser qtmultimedia5-dev \
        libqt5multimedia5-plugins \
        libssh2-1-dev libssl-dev
else
    info "qmake ✓ → $(command -v qmake)"
fi

# ─────────────────────────────────────────
# Step 2: GStreamer
# ─────────────────────────────────────────
info "[2/6] 检查 GStreamer..."

if ! dpkg -l 2>/dev/null | grep -q gstreamer1.0-plugins-good; then
    apt-get install -y \
        gstreamer1.0-tools \
        gstreamer1.0-plugins-base \
        gstreamer1.0-plugins-good \
        gstreamer1.0-plugins-bad \
        gstreamer1.0-plugins-ugly \
        gstreamer1.0-libav \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        gstreamer1.0-qt5 \
        v4l-utils
    info "GStreamer 安装完成"
else
    info "GStreamer ✓"
fi

# ─────────────────────────────────────────
# Step 3: 编译
# ─────────────────────────────────────────
info "[3/6] 编译..."

rm -rf "${OUT_DIR}"

qmake DualRecordService.pro CONFIG+=release
make -j$(nproc)

[ -f "${BUILD_BIN}" ] || err "编译失败"

# ─────────────────────────────────────────
# Step 4: 构建 deb
# ─────────────────────────────────────────
info "[4/6] 构建目录..."

rm -rf "${PKG_DIR}"

mkdir -p "${INSTALL_DIR}/bin"
mkdir -p "${INSTALL_DIR}/share/icons"
mkdir -p "${PKG_DIR}/DEBIAN"
mkdir -p "${PKG_DIR}/usr/share/applications"

cp -f "${BUILD_BIN}" "${INSTALL_DIR}/bin/DualRecordService"
chmod +x "${INSTALL_DIR}/bin/DualRecordService"

# 图标
ICON_DIR="${PROJECT_ROOT}/icons/png"
for SIZE in 16 24 32 48 64 128 256 512; do
    mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/${SIZE}x${SIZE}/apps"
    [ -f "${ICON_DIR}/${SIZE}x${SIZE}.png" ] && \
    cp -f "${ICON_DIR}/${SIZE}x${SIZE}.png" \
    "${PKG_DIR}/usr/share/icons/hicolor/${SIZE}x${SIZE}/apps/${APP_NAME}.png"
done

# ─────────────────────────────────────────
# Step 5: 文件生成
# ─────────────────────────────────────────
info "[5/6] 生成配置..."

# desktop
cat > "${PKG_DIR}/usr/share/applications/${APP_NAME}.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=${APP_DISPLAY}
Exec=/opt/dualrecord/bin/DualRecordService
Icon=${APP_NAME}
Terminal=false
X-GNOME-Autostart-enabled=true
EOF

# control
cat > "${PKG_DIR}/DEBIAN/control" <<EOF
Package: ${APP_NAME}
Version: ${APP_VERSION}
Architecture: ${APP_ARCH}
Maintainer: ${MAINTAINER}
Depends: libqt5core5a, libqt5gui5, libqt5widgets5, libqt5network5, libqt5multimedia5
Section: misc
Priority: optional
Description: ${DESCRIPTION}
EOF

# ─────────────────────────────────────────
# postinst（关键修复）
# ─────────────────────────────────────────
cat > "${PKG_DIR}/DEBIAN/postinst" << 'POSTINST'
#!/bin/bash

APP_BIN="/opt/dualrecord/bin/DualRecordService"

get_user() {
    u=$(logname 2>/dev/null)
    [ -z "$u" ] && u=$SUDO_USER
    [ -z "$u" ] && u=$(who | awk 'NR==1{print $1}')
    [ -z "$u" ] && u=$USER
    echo "$u"
}

USER_NAME=$(get_user)
USER_HOME=$(getent passwd "$USER_NAME" | cut -d: -f6)

echo "[INFO] 用户: $USER_NAME"

# 创建 autostart
if [ "$USER_NAME" != "root" ]; then
    AUTOSTART="$USER_HOME/.config/autostart"
    mkdir -p "$AUTOSTART"

    cat > "$AUTOSTART/dualrecord.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=银行双录控件
Exec=${APP_BIN}
Icon=dualrecord
X-GNOME-Autostart-enabled=true
EOF

    chown -R "$USER_NAME:$USER_NAME" "$AUTOSTART"
fi

# 🚀 立即启动（关键）
if [ "$USER_NAME" != "root" ]; then
    echo "[INFO] 尝试启动 GUI..."

    su - "$USER_NAME" -c "
        export DISPLAY=:0
        export XDG_RUNTIME_DIR=/run/user/$(id -u $USER_NAME)
        nohup ${APP_BIN} >/dev/null 2>&1 &
    " || true
fi

echo "[INFO] 安装完成"
POSTINST

chmod 755 "${PKG_DIR}/DEBIAN/postinst"

# ─────────────────────────────────────────
# Step 6: 打包
# ─────────────────────────────────────────
info "[6/6] 打包..."

mkdir -p "${RELEASE_DIR}"

DEB_FILE="${RELEASE_DIR}/${APP_NAME}_${APP_VERSION}_${APP_ARCH}.deb"
dpkg-deb --build "${PKG_DIR}" "${DEB_FILE}"

echo ""
echo "=========================================="
echo -e "  ${GREEN}完成！${NC}"
echo "=========================================="
echo "  ${DEB_FILE}"
echo ""

# 清理
rm -rf Makefile .qmake.stash out/