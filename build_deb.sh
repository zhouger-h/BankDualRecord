#!/bin/bash
# ═══════════════════════════════════════════════════════════════
# 银行柜面双录控件 - dpkg .deb 安装包打包脚本
# 目标平台：银河麒麟 V10 / 中标麒麟 / UOS（x86_64）
# 用法：bash build_deb.sh
# ═══════════════════════════════════════════════════════════════
set -e

APP_NAME="dualrecord"
APP_DISPLAY="银行柜面双录控件"
APP_VERSION="1.0.0"
APP_ARCH="amd64"
MAINTAINER="SiroInfo <support@siro-info.com>"
DESCRIPTION="银行柜面双录控件 - 基于Qt5/GStreamer的双路音视频录制服务"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEB_ROOT="${SCRIPT_DIR}/deb_build"
PKG_DIR="${DEB_ROOT}/${APP_NAME}_${APP_VERSION}_${APP_ARCH}"
INSTALL_DIR="${PKG_DIR}/opt/dualrecord"
BINARY="${SCRIPT_DIR}/DualRecordService"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
info()    { echo -e "${GREEN}[INFO]${NC} $1"; }
warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error()   { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

echo ""
echo "=========================================================="
echo "  银行柜面双录控件 - .deb 打包脚本 v${APP_VERSION}"
echo "=========================================================="
echo ""

# ── Step 1: 编译 ──────────────────────────────────────────────
info "[1/5] 编译程序..."
cd "${SCRIPT_DIR}"

if ! command -v qmake &>/dev/null; then
    error "未找到 qmake，请先运行 bash build_kylin.sh 安装依赖"
fi

rm -f Makefile DualRecordService
rm -rf build/
qmake DualRecordService.pro CONFIG+=release
make -j$(nproc)

[ -f "${BINARY}" ] || error "编译失败，未找到产物: ${BINARY}"
info "编译完成 ✓  →  ${BINARY}"

# ── Step 2: 清理并创建 deb 目录结构 ──────────────────────────
info "[2/5] 创建 deb 目录结构..."
rm -rf "${PKG_DIR}"
mkdir -p "${INSTALL_DIR}/bin"
mkdir -p "${INSTALL_DIR}/share/icons"
mkdir -p "${PKG_DIR}/etc/systemd/system"
mkdir -p "${PKG_DIR}/usr/share/applications"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/16x16/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/24x24/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/32x32/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/48x48/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/64x64/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/128x128/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/512x512/apps"
mkdir -p "${PKG_DIR}/DEBIAN"

# ── Step 3: 复制程序文件 ──────────────────────────────────────
info "[3/5] 复制文件..."

# 主程序
cp -f "${BINARY}" "${INSTALL_DIR}/bin/DualRecordService"
chmod +x "${INSTALL_DIR}/bin/DualRecordService"

# 图标：自动从 icons/png/ 寻找各尺寸 PNG
ICON_DIR="${SCRIPT_DIR}/icons/png"
ICON_SIZES="16 24 32 48 64 128 256 512"
ICON_FOUND=0
for SIZE in ${ICON_SIZES}; do
    SRC="${ICON_DIR}/${SIZE}x${SIZE}.png"
    if [ -f "${SRC}" ]; then
        cp -f "${SRC}" "${PKG_DIR}/usr/share/icons/hicolor/${SIZE}x${SIZE}/apps/${APP_NAME}.png"
        ICON_FOUND=$((ICON_FOUND + 1))
    fi
done
[ ${ICON_FOUND} -gt 0 ] && info "图标已安装（${ICON_FOUND} 个尺寸）" || warning "未找到 PNG 图标，将使用默认图标"

# 图标：额外复制 256x256 到 /opt/dualrecord/share/icons/（程序运行时直接加载用）
if [ -f "${ICON_DIR}/256x256.png" ]; then
    cp -f "${ICON_DIR}/256x256.png" "${INSTALL_DIR}/share/icons/256x256.png"
fi

# .desktop 启动文件
cat > "${PKG_DIR}/usr/share/applications/${APP_NAME}.desktop" << 'DESKTOP'
[Desktop Entry]
Type=Application
Name=银行双录控件
Name[zh_CN]=银行双录控件
Comment=Bank Counter Dual Recording Service
Exec=/opt/dualrecord/bin/DualRecordService
Icon=dualrecord
Terminal=false
Categories=Application;Utility;
StartupNotify=false
StartupWMClass=BankDualRecord
X-GNOME-Autostart-enabled=true
DESKTOP

# systemd 服务文件（占位符，postinst 脚本运行时替换实际用户）
cat > "${PKG_DIR}/etc/systemd/system/${APP_NAME}.service" << 'SERVICE'
[Unit]
Description=Bank Counter Dual Recording Service
After=network.target graphical-session.target
Wants=graphical-session.target

[Service]
Type=simple
User=DUALRECORD_USER
Environment=DISPLAY=:0
Environment=XAUTHORITY=DUALRECORD_HOME/.Xauthority
ExecStart=/opt/dualrecord/bin/DualRecordService
Restart=on-failure
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=graphical.target
SERVICE

# ── Step 4: 生成 DEBIAN 控制文件 ─────────────────────────────
info "[4/5] 生成 DEBIAN 控制文件..."

# ---- control ----
cat > "${PKG_DIR}/DEBIAN/control" << EOF
Package: ${APP_NAME}
Version: ${APP_VERSION}
Architecture: ${APP_ARCH}
Maintainer: ${MAINTAINER}
Depends: libqt5core5a, libqt5gui5, libqt5widgets5, libqt5network5, libqt5multimedia5, libqt5multimediawidgets5, libssl1.1 | libssl3, libssh2-1, gstreamer1.0-plugins-good, gstreamer1.0-plugins-base, libgstreamer1.0-0
Recommends: gstreamer1.0-plugins-bad, gstreamer1.0-libav, gstreamer1.0-qt5, v4l-utils
Section: misc
Priority: optional
Description: ${DESCRIPTION}
 基于 Qt5 和 GStreamer 实现双路（摄像头+屏幕）音视频同步录制，
 提供 HTTP REST 接口供柜面业务系统集成调用，
 支持 SFTP/FTP 自动上传，内置初始化配置向导。
EOF

# ---- postinst（安装后执行）----
cat > "${PKG_DIR}/DEBIAN/postinst" << 'POSTINST'
#!/bin/bash
# 安装后脚本：注册 systemd 服务、配置 XDG 自启动、立即启动

APP_NAME="dualrecord"
SERVICE_FILE="/etc/systemd/system/${APP_NAME}.service"
INSTALL_BIN="/opt/dualrecord/bin/DualRecordService"

# 获取实际桌面登录用户
get_desktop_user() {
    local u=""
    u=$(logname 2>/dev/null) || true
    [ -z "$u" ] || [ "$u" = "root" ] && u="${SUDO_USER}"
    [ -z "$u" ] || [ "$u" = "root" ] && u=$(who 2>/dev/null | grep -v root | awk 'NR==1{print $1}') || true
    [ -z "$u" ] && u="${USER}"
    echo "$u"
}

CURRENT_USER=$(get_desktop_user)
if [ -z "${CURRENT_USER}" ] || [ "${CURRENT_USER}" = "root" ]; then
    echo "[WARN] 无法确定桌面用户，服务将以 root 运行（不推荐）"
    CURRENT_USER="root"
fi

USER_HOME=$(getent passwd "${CURRENT_USER}" 2>/dev/null | cut -d: -f6)
[ -z "${USER_HOME}" ] && USER_HOME=$(eval echo ~${CURRENT_USER} 2>/dev/null)
[ -z "${USER_HOME}" ] && USER_HOME="/home/${CURRENT_USER}"

echo "[INFO] 配置用户: ${CURRENT_USER}  主目录: ${USER_HOME}"

# 替换 service 文件中的占位符
sed -i "s|DUALRECORD_USER|${CURRENT_USER}|g"   "${SERVICE_FILE}"
sed -i "s|DUALRECORD_HOME|${USER_HOME}|g"       "${SERVICE_FILE}"

# 创建用户工作目录
if [ "${CURRENT_USER}" != "root" ]; then
    mkdir -p "${USER_HOME}/BankDualRecord/videos"
    mkdir -p "${USER_HOME}/BankDualRecord/logs"
    chown -R "${CURRENT_USER}:${CURRENT_USER}" "${USER_HOME}/BankDualRecord" 2>/dev/null || true
fi

# XDG 自启动（兜底，在没有 systemd 图形会话的情况下也能自动启动）
if [ "${CURRENT_USER}" != "root" ] && [ -d "${USER_HOME}" ]; then
    AUTOSTART_DIR="${USER_HOME}/.config/autostart"
    mkdir -p "${AUTOSTART_DIR}"
    cat > "${AUTOSTART_DIR}/dualrecord.desktop" << EOF
[Desktop Entry]
Type=Application
Name=银行双录控件
Exec=${INSTALL_BIN}
Icon=dualrecord
StartupWMClass=BankDualRecord
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
EOF
    chown -R "${CURRENT_USER}:${CURRENT_USER}" "${AUTOSTART_DIR}" 2>/dev/null || true
fi

# 注册并启动服务
systemctl daemon-reload 2>/dev/null || true
systemctl enable "${APP_NAME}.service" 2>/dev/null || true

# 停止旧进程（支持重新安装场景）
systemctl stop "${APP_NAME}.service" 2>/dev/null || true
pkill -9 -f "DualRecordService" 2>/dev/null || true
sleep 1

# 尝试 systemctl start
systemctl start "${APP_NAME}.service" 2>/dev/null
START_RC=$?

if [ ${START_RC} -eq 0 ]; then
    sleep 1
    if systemctl is-active --quiet "${APP_NAME}.service" 2>/dev/null; then
        echo "[INFO] 服务启动成功 ✓"
    else
        echo "[WARN] 服务已提交启动，正在初始化..."
        START_RC=1
    fi
fi

# fallback：直接以用户身份拉起进程
if [ ${START_RC} -ne 0 ]; then
    echo "[WARN] systemctl 启动未就绪，尝试直接启动进程..."
    DISPLAY_VAR=$(su - "${CURRENT_USER}" -c 'echo $DISPLAY' 2>/dev/null || echo ":0")
    nohup su - "${CURRENT_USER}" -c \
        "DISPLAY=${DISPLAY_VAR} XAUTHORITY=${USER_HOME}/.Xauthority ${INSTALL_BIN}" \
        >/tmp/dualrecord_start.log 2>&1 &
    sleep 2
    if pgrep -f "DualRecordService" >/dev/null 2>&1; then
        echo "[INFO] 服务直接启动成功 ✓"
    else
        echo "[WARN] 即时启动未成功，开机登录后将自动运行"
    fi
fi

# 更新图标缓存
gtk-update-icon-cache /usr/share/icons/hicolor 2>/dev/null || true
update-desktop-database 2>/dev/null || true

echo ""
echo "================================================"
echo "  银行柜面双录控件 安装完成！"
echo "================================================"
echo "  程序路径: /opt/dualrecord/bin/DualRecordService"
echo "  HTTP端口: 9528 (可在初始化向导中修改)"
echo "  视频目录: ${USER_HOME}/BankDualRecord/videos"
echo "  查看日志: journalctl -u dualrecord -f"
echo "================================================"
POSTINST
chmod 755 "${PKG_DIR}/DEBIAN/postinst"

# ---- prerm（卸载前执行）----
cat > "${PKG_DIR}/DEBIAN/prerm" << 'PRERM'
#!/bin/bash
# 卸载前脚本：停止服务
APP_NAME="dualrecord"
echo "[INFO] 停止双录服务..."
systemctl stop "${APP_NAME}.service"   2>/dev/null || true
systemctl disable "${APP_NAME}.service" 2>/dev/null || true
pkill -9 -f "DualRecordService"         2>/dev/null || true
PRERM
chmod 755 "${PKG_DIR}/DEBIAN/prerm"

# ---- postrm（卸载后执行）----
cat > "${PKG_DIR}/DEBIAN/postrm" << 'POSTRM'
#!/bin/bash
# 卸载后脚本：清理残留文件
if [ "$1" = "remove" ] || [ "$1" = "purge" ]; then
    rm -f /etc/systemd/system/dualrecord.service
    systemctl daemon-reload 2>/dev/null || true
    # 清理所有用户的自启动条目
    find /home -maxdepth 4 -name "dualrecord.desktop" \
         -path "*/autostart/*" -delete 2>/dev/null || true
    # 更新图标缓存
    gtk-update-icon-cache /usr/share/icons/hicolor 2>/dev/null || true
    update-desktop-database 2>/dev/null || true
    echo "[INFO] 卸载完成。视频缓存保留在 ~/BankDualRecord 中。"
fi
POSTRM
chmod 755 "${PKG_DIR}/DEBIAN/postrm"

# ── Step 5: 打包 .deb ─────────────────────────────────────────
info "[5/5] 打包 .deb..."
cd "${DEB_ROOT}"
DEB_FILE="${SCRIPT_DIR}/${APP_NAME}_${APP_VERSION}_${APP_ARCH}.deb"
dpkg-deb --build "${PKG_DIR}" "${DEB_FILE}"

echo ""
echo "=========================================================="
echo -e "  ${GREEN}打包完成！${NC}"
echo "=========================================================="
echo "  安装包: ${DEB_FILE}"
SIZE=$(du -sh "${DEB_FILE}" | cut -f1)
echo "  大小:   ${SIZE}"
echo ""
echo "  安装命令:"
echo "    sudo dpkg -i ${APP_NAME}_${APP_VERSION}_${APP_ARCH}.deb"
echo ""
echo "  安装后若缺少依赖，执行:"
echo "    sudo apt-get install -f"
echo ""
echo "  卸载命令:"
echo "    sudo dpkg -r ${APP_NAME}"
echo "=========================================================="
echo ""
