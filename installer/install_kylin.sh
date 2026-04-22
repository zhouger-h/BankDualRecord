#!/bin/bash
# ═══════════════════════════════════════════════════════
# 银行柜面双录控件 - 麒麟/信创一键安装脚本
# 支持：银河麒麟 V10 / 中标麒麟 / UOS / 统信
# ═══════════════════════════════════════════════════════

# 注意：不使用 set -e，所有关键步骤自行处理错误，避免 systemctl stop 等命令
# 在卸载后重新安装场景下找不到旧服务时错误退出

APP_NAME="BankDualRecord"
APP_VERSION="1.0.0"
INSTALL_DIR="/opt/dualrecord"
SERVICE_NAME="dualrecord"

# ─── 获取实际登录用户 ───
# 优先级：logname > SUDO_USER > who 取第一个 > USER
CURRENT_USER=""
CURRENT_USER=$(logname 2>/dev/null) || true
if [ -z "${CURRENT_USER}" ] || [ "${CURRENT_USER}" = "root" ]; then
    CURRENT_USER="${SUDO_USER}"
fi
if [ -z "${CURRENT_USER}" ] || [ "${CURRENT_USER}" = "root" ]; then
    CURRENT_USER=$(who 2>/dev/null | grep -v root | awk 'NR==1{print $1}') || true
fi
if [ -z "${CURRENT_USER}" ]; then
    CURRENT_USER="${USER}"
fi

# 获取用户主目录
if [ -n "${CURRENT_USER}" ] && [ "${CURRENT_USER}" != "root" ]; then
    USER_HOME=$(getent passwd "${CURRENT_USER}" 2>/dev/null | cut -d: -f6)
    [ -z "${USER_HOME}" ] && USER_HOME=$(eval echo ~${CURRENT_USER} 2>/dev/null)
else
    USER_HOME="/root"
    CURRENT_USER="root"
fi

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
info()    { echo -e "${GREEN}[INFO]${NC} $1"; }
warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error()   { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# ─── 权限检查 ───
if [ "$EUID" -ne 0 ]; then
    error "请使用 sudo 运行此脚本: sudo bash $0"
fi

info "开始安装 银行柜面双录控件 v${APP_VERSION}"
info "安装目录: ${INSTALL_DIR}"
info "运行用户: ${CURRENT_USER}  (家目录: ${USER_HOME})"

# ─── 强制停止旧进程（卸载后重装场景：service 文件已删除，用 pkill 兜底）───
info "清理旧进程..."
systemctl stop "${SERVICE_NAME}.service" 2>/dev/null || true
pkill -9 -f "DualRecordService" 2>/dev/null || true
sleep 1

# ─── 检测系统依赖 ───
check_dependency() {
    if ! ldconfig -p 2>/dev/null | grep -q "$1"; then
        warning "未找到 $1，尝试自动安装..."
        if command -v apt-get &>/dev/null; then
            apt-get install -y "$2" 2>/dev/null || warning "自动安装失败，请手动安装 $2"
        elif command -v yum &>/dev/null; then
            yum install -y "$2" 2>/dev/null || warning "自动安装失败，请手动安装 $2"
        elif command -v dnf &>/dev/null; then
            dnf install -y "$2" 2>/dev/null || warning "自动安装失败，请手动安装 $2"
        fi
    else
        info "依赖 $1 已满足"
    fi
}

info "检查系统依赖..."
check_dependency "libssl"   "libssl-dev"
check_dependency "libssh2"  "libssh2-1"

# GStreamer 是 Qt 多媒体/摄像头在 Linux 下的底层驱动
info "检查 GStreamer 多媒体框架..."
if ! dpkg -l 2>/dev/null | grep -q gstreamer1.0-plugins-good && \
   ! rpm -qa 2>/dev/null | grep -q gstreamer1; then
    warning "未检测到 GStreamer，正在安装..."
    if command -v apt-get &>/dev/null; then
        apt-get install -y \
            gstreamer1.0-tools \
            gstreamer1.0-plugins-base \
            gstreamer1.0-plugins-good \
            gstreamer1.0-plugins-bad \
            gstreamer1.0-plugins-ugly \
            gstreamer1.0-libav \
            v4l-utils 2>/dev/null || warning "GStreamer 安装失败，摄像头可能无法使用"
    elif command -v yum &>/dev/null || command -v dnf &>/dev/null; then
        PKG_MGR=$(command -v dnf 2>/dev/null || command -v yum)
        $PKG_MGR install -y \
            gstreamer1 gstreamer1-plugins-base \
            gstreamer1-plugins-good gstreamer1-plugins-bad-free \
            v4l-utils 2>/dev/null || warning "GStreamer 安装失败，摄像头可能无法使用"
    fi
else
    info "GStreamer 已安装 ✓"
fi

# ─── 创建安装目录 ───
mkdir -p "${INSTALL_DIR}/bin"
mkdir -p "${INSTALL_DIR}/lib"
mkdir -p "${INSTALL_DIR}/plugins"
mkdir -p "${INSTALL_DIR}/config"

# ─── 复制程序文件 ───
info "复制程序文件..."
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ ! -f "${SCRIPT_DIR}/bin/DualRecordService" ]; then
    error "未找到可执行文件: ${SCRIPT_DIR}/bin/DualRecordService，请先执行编译"
fi

cp -f "${SCRIPT_DIR}/bin/DualRecordService"  "${INSTALL_DIR}/bin/"
cp -rf "${SCRIPT_DIR}/lib/"*                  "${INSTALL_DIR}/lib/"  2>/dev/null || true
cp -rf "${SCRIPT_DIR}/plugins/"*              "${INSTALL_DIR}/plugins/" 2>/dev/null || true
chmod +x "${INSTALL_DIR}/bin/DualRecordService"
info "程序文件复制完成 ✓"

# ─── 配置动态库路径 ───
echo "${INSTALL_DIR}/lib" > /etc/ld.so.conf.d/dualrecord.conf
ldconfig
info "动态库路径已配置 ✓"

# ─── 创建用户工作目录 ───
if [ "${CURRENT_USER}" != "root" ]; then
    mkdir -p "${USER_HOME}/BankDualRecord/videos"
    mkdir -p "${USER_HOME}/BankDualRecord/logs"
    chown -R "${CURRENT_USER}:${CURRENT_USER}" "${USER_HOME}/BankDualRecord" 2>/dev/null || true
    info "用户工作目录: ${USER_HOME}/BankDualRecord"
fi

# ─── 创建 systemd 服务（system 级，随图形会话启动）───
info "配置开机自启 (systemd)..."
cat > "/etc/systemd/system/${SERVICE_NAME}.service" << EOF
[Unit]
Description=Bank Counter Dual Recording Service
After=network.target graphical-session.target
Wants=graphical-session.target

[Service]
Type=simple
User=${CURRENT_USER}
Environment=DISPLAY=:0
Environment=XAUTHORITY=${USER_HOME}/.Xauthority
ExecStart=${INSTALL_DIR}/bin/DualRecordService
Restart=on-failure
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=graphical.target
EOF

systemctl daemon-reload
systemctl enable "${SERVICE_NAME}.service"
info "systemd 服务已注册并启用 ✓"

# ─── 创建 XDG 桌面自启动（兜底方案：图形会话登录时自动启动）───
if [ "${CURRENT_USER}" != "root" ] && [ -d "${USER_HOME}" ]; then
    AUTOSTART_DIR="${USER_HOME}/.config/autostart"
    mkdir -p "${AUTOSTART_DIR}"
    cat > "${AUTOSTART_DIR}/${APP_NAME}.desktop" << EOF
[Desktop Entry]
Type=Application
Name=银行双录控件
Comment=Bank Counter Dual Recording Service
Exec=${INSTALL_DIR}/bin/DualRecordService
Icon=${INSTALL_DIR}/config/app.png
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
EOF
    chown -R "${CURRENT_USER}:${CURRENT_USER}" "${AUTOSTART_DIR}" 2>/dev/null || true
    info "XDG 桌面自启动已配置 ✓"
fi

# ─── 创建桌面快捷方式 ───
if [ "${CURRENT_USER}" != "root" ] && [ -d "${USER_HOME}/Desktop" ]; then
    cat > "${USER_HOME}/Desktop/${APP_NAME}.desktop" << EOF
[Desktop Entry]
Type=Application
Name=银行双录控件
Comment=Bank Counter Dual Recording Service
Exec=${INSTALL_DIR}/bin/DualRecordService
Icon=${INSTALL_DIR}/config/app.png
Terminal=false
Categories=Application;
EOF
    chmod +x "${USER_HOME}/Desktop/${APP_NAME}.desktop"
    chown "${CURRENT_USER}:${CURRENT_USER}" "${USER_HOME}/Desktop/${APP_NAME}.desktop" 2>/dev/null || true
    info "桌面快捷方式已创建 ✓"
fi

# ─── 创建卸载脚本 ───
cat > "${INSTALL_DIR}/uninstall.sh" << 'UNINSTALL'
#!/bin/bash
echo "正在卸载银行双录控件..."
systemctl stop dualrecord.service   2>/dev/null || true
systemctl disable dualrecord.service 2>/dev/null || true
pkill -9 -f "DualRecordService"     2>/dev/null || true
rm -f /etc/systemd/system/dualrecord.service
rm -f /etc/ld.so.conf.d/dualrecord.conf
ldconfig 2>/dev/null || true
systemctl daemon-reload              2>/dev/null || true
# 清理桌面自启动条目
find /home -name "BankDualRecord.desktop" -path "*/autostart/*" -delete 2>/dev/null || true
find /home -name "BankDualRecord.desktop" -path "*/Desktop/*"   -delete 2>/dev/null || true
rm -rf /opt/dualrecord
echo "卸载完成。视频缓存保留在 ~/BankDualRecord 中。"
UNINSTALL
chmod +x "${INSTALL_DIR}/uninstall.sh"
info "卸载脚本已创建: ${INSTALL_DIR}/uninstall.sh"

# ─── 立即启动服务 ───
info "尝试立即启动服务..."
systemctl daemon-reload

# 方案一：通过 systemctl start 启动（需要图形会话已就绪）
START_OK=false
systemctl start "${SERVICE_NAME}.service" 2>/dev/null && START_OK=true || true

if [ "${START_OK}" = "true" ]; then
    sleep 1
    if systemctl is-active --quiet "${SERVICE_NAME}.service" 2>/dev/null; then
        info "服务启动成功 ✓  (systemctl)"
    else
        # systemctl start 返回0但服务还没 active，可能在初始化中
        info "服务已提交启动，正在初始化..."
        START_OK=false
    fi
fi

# 方案二：systemctl 启动失败时，以实际用户身份直接运行进程（适用于安装时没有图形会话的场景）
if [ "${START_OK}" = "false" ]; then
    warning "systemctl 启动未就绪，尝试直接启动进程..."
    if [ "${CURRENT_USER}" != "root" ]; then
        # 获取当前桌面显示器
        DISPLAY_VAR=$(su - "${CURRENT_USER}" -c 'echo $DISPLAY' 2>/dev/null || echo ":0")
        XAUTH_FILE="${USER_HOME}/.Xauthority"
        nohup su - "${CURRENT_USER}" -c \
            "DISPLAY=${DISPLAY_VAR} XAUTHORITY=${XAUTH_FILE} ${INSTALL_DIR}/bin/DualRecordService" \
            >/tmp/dualrecord_start.log 2>&1 &
    else
        nohup "${INSTALL_DIR}/bin/DualRecordService" \
            >/tmp/dualrecord_start.log 2>&1 &
    fi
    sleep 2
    if pgrep -f "DualRecordService" >/dev/null 2>&1; then
        info "服务直接启动成功 ✓  (进程已运行)"
    else
        warning "即时启动未成功，开机自启已配置，重新登录后服务将自动运行"
        warning "日志: /tmp/dualrecord_start.log"
    fi
fi

echo ""
echo -e "${GREEN}═══════════════════════════════════════════════${NC}"
echo -e "${GREEN}  安装完成！银行柜面双录控件 v${APP_VERSION}    ${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════${NC}"
echo -e "  程序路径:   ${INSTALL_DIR}/bin/DualRecordService"
echo -e "  运行用户:   ${CURRENT_USER}"
echo -e "  HTTP端口:   9528 (可在初始化向导中修改)"
echo -e "  视频缓存:   ${USER_HOME}/BankDualRecord/videos"
echo -e "  查看日志:   journalctl -u ${SERVICE_NAME} -f"
echo -e "  卸载:       sudo bash ${INSTALL_DIR}/uninstall.sh"
echo ""
