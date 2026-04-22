# 银行柜面双录控件

> 专为银行内网设计 · 无商业版权问题 · 支持 Windows 32位 + 银河麒麟信创

[![许可证](https://img.shields.io/badge/业务代码-MIT-green)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-LGPL_2.1-blue)](https://www.qt.io)
[![平台](https://img.shields.io/badge/平台-Win32_|_麒麟-orange)]()

---

## 📋 功能概述

| 功能 | 说明 |
|------|------|
| 🎬 音视频录制 | H.264视频 + MP3音频，支持720P/1080P |
| 📡 TCP接口 | JSON协议，供柜面系统实时控制 |
| ☁ FTP/SFTP上传 | 自动上传，带宽控制，失败续传 |
| 🗂 本地管理 | 视频浏览、播放、删除 |
| 🔄 开机自启 | Windows注册表 / 麒麟systemd |
| 🎯 单实例 | 防止重复启动 |
| 🖥 系统托盘 | 后台运行，不占用任务栏 |

## 🚀 快速开始

### 方式一：使用演示Demo（无需编译）

浏览器直接打开 `demo/index.html` 即可体验交互界面：

```
dual-record-system/demo/index.html   ← 录制主界面（Web演示）
dual-record-system/demo/player.html  ← 独立播放器
```

> ⚠️ 注意：Web演示使用浏览器WebRTC API，仅用于界面交互演示。实际功能需编译C++控件。

---

### 方式二：编译安装正式控件

#### Windows 32位

**步骤1：编译Qt程序**

```bat
REM 需要安装 Qt 5.15.2 (MinGW 32-bit) + NSIS
REM 打开 Qt 5.15.2 (MinGW 8.1.0 32-bit) 命令行
cd dual-record-system
qmake DualRecordService.pro
mingw32-make release

REM 编译完成后生成 release/DualRecordService.exe
```

**步骤2：打包安装程序**

```bat
REM 使用NSIS编译安装脚本
"C:\Program Files (x86)\NSIS\makensis.exe" installer\install_windows.nsi

REM 生成 BankDualRecord_Setup_v1.0.0_x86.exe
```

**步骤3：安装运行**

```bat
REM 以管理员身份运行生成的安装包
BankDualRecord_Setup_v1.0.0_x86.exe
```

---

#### 银河麒麟 / 信创

```bash
# 安装依赖
sudo apt-get install qt5-default libqt5multimedia5-dev libssh2-1-dev openssl-dev

# 编译
cd dual-record-system
qmake DualRecordService.pro
make -j$(nproc)

# 安装
sudo bash installer/install_kylin.sh
```

安装完成后程序自动启动，首次运行弹出初始化向导。

## 🔌 TCP接口快速示例

```python
import socket, struct, json

# 连接
sock = socket.socket(); sock.connect(('127.0.0.1', 9527))

def send(cmd, params={}):
    body = json.dumps({'cmd':cmd,'params':params}).encode()
    sock.sendall(struct.pack('>I',len(body))+body)
    n = struct.unpack('>I', sock.recv(4))[0]
    return json.loads(sock.recv(n))

# 开始录制
send('START', {'task_id':'TRX001', 'cust_name':'张三'})
# 停止录制
resp = send('STOP')
print('视频保存至:', resp['file_path'])
```

## 📁 项目结构

```
dual-record-system/
├── core/                    # 核心源码 (C++/Qt)
│   ├── DualRecordWidget.{h,cpp}  # 录制UI控件
│   ├── TcpServer.{h,cpp}         # TCP通信服务
│   ├── UploadManager.{h,cpp}     # FTP/SFTP上传管理
│   ├── InitConfigDialog.{h,cpp}  # 初始化配置向导
│   ├── AutoStartHelper.{h,cpp}   # 开机自启注册
│   └── main.cpp                  # 程序入口
├── demo/                    # 演示程序 (C++/Qt + HTML)
│   ├── DemoWindow.{h,cpp}        # Qt演示界面
│   ├── index.html                # Web交互演示（可直接浏览器打开）
│   └── demo_main.cpp
├── installer/               # 安装脚本
│   ├── install_windows.nsi       # NSIS Windows安装脚本
│   └── install_kylin.sh          # 麒麟一键安装脚本
├── docs/                    # 文档
│   ├── 设计文档.md
│   └── API接口说明.md
├── service/                 # 系统服务相关
│   └── AutoStartHelper.{h,cpp}
└── DualRecordService.pro    # Qt项目文件
```

## 📖 文档

- [设计文档](docs/设计文档.md) — 架构设计、模块说明
- [API接口说明](docs/API接口说明.md) — TCP协议、命令参考
- [Demo演示](demo/index.html) — 在浏览器中打开即可体验交互演示

## 🔧 编译要求

| 平台 | 工具链 | Qt版本 |
|------|--------|--------|
| Windows 32位 | MinGW-w64 (i686) / MSVC 2019 x86 | Qt 5.15.2 |
| 银河麒麟 x86_64 | GCC 9+ | Qt 5.15.x |
| 银河麒麟 aarch64 | GCC aarch64 | Qt 5.15.x |

## 📄 版权说明

- **业务代码**：MIT License（可自由商业使用）
- **Qt框架**：LGPL 2.1（动态链接，无需开源）
- **libssh2**：BSD 2-Clause（无限制）
- **OpenSSL**：Apache 2.0（无限制）
- **NSIS**：zlib/libpng（免费）

> ✅ 所有依赖均无商业版权问题，无需任何第三方授权，可在银行内网安全部署。
