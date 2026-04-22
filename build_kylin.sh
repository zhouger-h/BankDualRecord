#!/bin/bash
# 银行柜面双录控件 - 麒麟编译脚本

set -e

echo "=========================================="
echo "  银行柜面双录控件 - 麒麟编译脚本"
echo "=========================================="
echo ""

# 检查依赖
echo "[1/4] 检查依赖..."
if ! command -v qmake &> /dev/null; then
    echo "[错误] 未找到 qmake，正在尝试安装 Qt 开发包..."
    apt-get update
    apt-get install -y \
        qt5-default qtbase5-dev qtmultimedia5-dev \
        libqt5multimedia5-plugins \
        libssh2-1-dev libssl-dev
fi

# 检查并安装 GStreamer（Qt 多媒体/摄像头在 Linux 下依赖 GStreamer）
# 必须安装的包说明：
#   gstreamer1.0-plugins-base  — vorbis(音频) + ogg(容器) + videoscale/videoconvert 等基础插件
#   gstreamer1.0-plugins-good  — vpx(vp8视频编解码) + v4l2src(摄像头输入) 等
#   gstreamer1.0-plugins-bad   — qtmultimedia5 的录制管道依赖
#   gstreamer1.0-libav         — 可选，提供 h264/aac/mp4 等 ffmpeg 编解码支持
echo "  检查 GStreamer..."
if ! dpkg -l 2>/dev/null | grep -q gstreamer1.0-plugins-good; then
    echo "  安装 GStreamer 插件..."
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
    echo "  GStreamer 安装完成"
else
    # 即使主包已安装，也补充确保 gstreamer1.0-qt5 在位
    apt-get install -y gstreamer1.0-qt5 v4l-utils 2>/dev/null || true
    echo "  GStreamer 已安装 ✓"
fi

# 清理
echo "[2/4] 清理旧文件..."
rm -f Makefile Makefile.Debug Makefile.Release
rm -f DualRecordService
rm -rf build/

# 生成 Makefile（指定 release 模式）
echo "[3/4] 生成 Makefile..."
qmake DualRecordService.pro CONFIG+=release

# 编译
echo "[4/4] 编译程序..."
make -j$(nproc)

echo ""
echo "=========================================="
echo "  编译完成!"
echo "=========================================="
echo ""
echo "输出文件:"
echo "  - 主程序: ./DualRecordService"
echo ""
echo "打包为 .deb 安装包（推荐）:"
echo "  bash build_deb.sh"
echo ""
echo "安装 .deb 包:"
echo "  sudo dpkg -i dualrecord_1.0.0_amd64.deb"
echo "  sudo apt-get install -f   # 自动补全依赖"
echo ""
