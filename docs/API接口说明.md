# 银行柜面双录控件 — TCP 接口说明

> 版本：2.1.0 | 协议：JSON over TCP | 默认端口：**9527**（可配置）

---

## 1. 帧格式

```
┌──────────────────┬────────────────────────────────────┐
│  4 字节 大端长度  │   JSON Body（UTF-8，无结尾换行）    │
└──────────────────┴────────────────────────────────────┘
```

**发送示例（Python）**

```python
import socket, json, struct

def send_cmd(sock, cmd, params={}):
    body = json.dumps({"cmd": cmd, "params": params}, ensure_ascii=False).encode()
    sock.sendall(struct.pack(">I", len(body)) + body)

s = socket.socket()
s.connect(("127.0.0.1", 9527))
# 吊起界面并预设文件名
send_cmd(s, "SHOW_WINDOW", {"file_name": "T20260331001_张三.mp4"})
# 或直接通过 START 指定文件名
send_cmd(s, "START", {"task_id": "T001", "cust_name": "张三", "file_name": "T001_张三.mp4"})
```

**发送示例（Qt/C++）**

```cpp
auto sendCmd = [&](QTcpSocket *sock, const QJsonObject &cmd) {
    QByteArray body = QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    quint32 len = body.size();
    QByteArray frame(4, 0);
    frame[0]=(len>>24)&0xFF; frame[1]=(len>>16)&0xFF;
    frame[2]=(len>>8) &0xFF; frame[3]=len&0xFF;
    sock->write(frame + body);
};
sendCmd(sock, {{"cmd","START"}, {"params", QJsonObject{{"task_id","T001"},{"cust_name","张三"}}}});
```

---

## 2. 命令列表

### 2.1 START — 开始录制

```json
// 请求（file_name 为可选字段）
{
  "cmd": "START",
  "params": {
    "task_id":  "T20260331001",   // 必填：任务ID
    "cust_name": "张三",           // 可选：客户姓名（仅记录用）
    "file_name": "T20260331001_张三.mp4"  // 可选：指定录制文件名
  }
}

// 响应
{
  "code": 0,
  "msg": "录制已启动",
  "task_id": "T20260331001",
  "file_name": "T20260331001_张三.mp4",  // 回显传入的 file_name；未传时为 null
  "ts": 1711859400000
}

// 主动事件（控件推送，含最终文件完整路径）
{
  "type": "event",
  "event": "RECORD_STARTED",
  "data": {
    "task_id": "T20260331001",
    "file_path": "D:\\DualRecord\\T20260331001_张三.mp4"  // 完整路径，file_name 已生效
  }
}
```

> **file_name 字段说明**
> - **传入 `file_name`**：录制文件以该名称保存到配置的缓存目录，路径为 `{video_cache_path}/{file_name}`
> - **不含扩展名时**：自动追加 `.mp4`（Windows）或 `.ogv`（麒麟/Linux）
> - **不传 `file_name`**：按默认规则生成文件名 `{task_id}_{yyyyMMdd_HHmmss}.mp4`
> - **最终文件路径** 在 `RECORD_STARTED` 事件的 `file_path` 字段中确认（摄像头就绪后触发）
>
> **行为**：打开摄像头 → 就绪后开始录制 → 推送 `RECORD_STARTED` 事件（含完整 file_path）

---

### 2.2 PAUSE — 暂停录制

```json
{"cmd": "PAUSE", "params": {}}
// 响应: {"code": 0, "msg": "录制已暂停"}
// 事件: {"type":"event","event":"RECORD_PAUSED","data":{"task_id":"..."}}
```

---

### 2.3 RESUME — 继续录制

```json
{"cmd": "RESUME", "params": {}}
// 响应: {"code": 0, "msg": "录制已继续"}
// 事件: {"type":"event","event":"RECORD_RESUMED","data":{"task_id":"..."}}
```

---

### 2.4 STOP — 停止录制

```json
{"cmd": "STOP", "params": {}}

// 响应
{"code": 0, "msg": "录制已停止",
 "file_path": "D:\\DualRecord\\T20260331001_20260331_143020.mp4"}

// 事件1: 录制停止
{"type":"event","event":"RECORD_STOPPED",
 "data":{"task_id":"T20260331001","file_path":"..."}}

// 事件2: 文件入上传队列
{"type":"event","event":"UPLOAD_QUEUED","data":{"file_path":"..."}}
```

> **行为**：停止录制 → 关闭摄像头 → 文件自动入上传队列

---

### 2.5 PLAY — 播放视频（弹出独立播放器窗口）

```json
{"cmd": "PLAY", "params": {"file_path": "D:\\DualRecord\\T20260331001_20260331_143020.mp4"}}

// 响应
{"code": 0, "msg": "播放器已启动", "file_path": "..."}
```

> **行为**：弹出独立 `PlayerWindow` 窗口播放指定文件，与录制界面完全分离

---

### 2.6 LIST — 获取视频列表

```json
{"cmd": "LIST", "params": {}}

// 响应
{
  "code": 0, "msg": "OK",
  "list": [
    {"name": "T001_20260331_143020.mp4", "path": "D:\\DualRecord\\T001_xxx.mp4",
     "size": 52428800, "modified": "2026-03-31T14:30:20"}
  ]
}
```

---

### 2.7 STATUS — 查询状态

```json
{"cmd": "STATUS", "params": {}}

// 响应
{
  "code": 0, "msg": "OK",
  "data": {
    "state": "IDLE",          // IDLE | RECORDING | PAUSED | STOPPED
    "file_path": ""           // 录制中时为当前文件路径
  }
}
```

---

### 2.8 PING — 心跳检测

```json
{"cmd": "PING", "params": {}}
// 响应: {"code": 0, "msg": "pong", "ts": 1711859400000}
```

---

### 2.9 SHOW_WINDOW — 显示主界面（吊起）

```json
// 请求（file_name 为可选字段）
{
  "cmd": "SHOW_WINDOW",
  "params": {
    "file_name": "T20260331001_张三.mp4"  // 可选：预设本次录制使用的文件名
  }
}

// 响应
{
  "code": 0,
  "msg": "主界面已显示",
  "file_name": "T20260331001_张三.mp4",  // 回显传入的 file_name；未传时为空字符串
  "ts": 1711859400000
}
```

> **`file_name` 字段说明**：与 `START` 命令的 `file_name` 含义相同。在吊起界面时预设文件名后，
> 操作员点击「开始录制」时将自动使用该文件名，无需再通过 `START` 命令传入。
>
> **典型流程**：`SHOW_WINDOW(file_name=xxx)` → 操作员点击「开始录制」→ `RECORD_STARTED` 事件（含完整路径）
>
> **清除时机**：关闭界面按钮、右上角叉、`HIDE_WINDOW` 命令均会清除预设文件名，下次吊起需重新传入。

---

### 2.10 HIDE_WINDOW — 隐藏主界面

```json
{"cmd": "HIDE_WINDOW", "params": {}}
// 响应: {"code": 0, "msg": "主界面已隐藏"}
// 副作用: 自动清除预设文件名（m_pendingFileName）
```

> **注意**：`HIDE_WINDOW` 与 `SHOW_WINDOW` 仅控制界面显隐，后台 TCP 服务始终运行。

---

## 3. 上传事件（主动推送）

上传过程中，控件会主动向所有已连接的柜面客户端推送进度事件：

```json
// 上传进度
{"type":"event","event":"UPLOAD_PROGRESS",
 "data":{"file_path":"D:\\DualRecord\\xxx.mp4","sent":1048576,"total":52428800}}

// 上传成功（本地文件已自动删除）
{"type":"event","event":"UPLOAD_DONE",
 "data":{"file_path":"D:\\DualRecord\\xxx.mp4","status":"success"}}

// 上传失败（本地文件保留，下次启动自动续传）
{"type":"event","event":"UPLOAD_FAILED",
 "data":{"file_path":"D:\\DualRecord\\xxx.mp4","error":"连接超时","retry":2}}
```

---

## 4. 错误码

| code | 含义              |
|------|-------------------|
| 0    | 成功              |
| -1   | 参数错误          |
| -2   | 状态错误（如录制中重复START）|
| -3   | 设备不可用        |
| -4   | 文件不存在        |
| -99  | 内部错误          |

---

## 5. 完整集成示例（C++ Qt）

```cpp
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>

class DualRecordClient : public QObject {
    Q_OBJECT
    QTcpSocket *m_sock;
    QByteArray  m_buf;
public:
    DualRecordClient(QObject *parent=nullptr) : QObject(parent) {
        m_sock = new QTcpSocket(this);
        connect(m_sock, &QTcpSocket::readyRead, this, &DualRecordClient::onRead);
        connect(m_sock, &QTcpSocket::connected,  this, [this]{ qDebug("已连接双录控件"); });
    }
    void connect(const QString &host="127.0.0.1", quint16 port=9527) {
        m_sock->connectToHost(host, port);
    }
    void send(const QJsonObject &cmd) {
        QByteArray body = QJsonDocument(cmd).toJson(QJsonDocument::Compact);
        quint32 len = body.size();
        QByteArray frame(4,0);
        frame[0]=(len>>24)&0xFF; frame[1]=(len>>16)&0xFF;
        frame[2]=(len>>8) &0xFF; frame[3]=len&0xFF;
        m_sock->write(frame+body);
    }
    // 吊起界面并预设文件名（用户点「开始录制」将使用该文件名）
    void showWindow(const QString &fileName = QString()) {
        QJsonObject p;
        if (!fileName.isEmpty()) p["file_name"] = fileName;
        send({{"cmd","SHOW_WINDOW"},{"params", p}});
    }
    // 开始录制（可选指定文件名）
    void startRecord(const QString &taskId, const QString &custName,
                     const QString &fileName = QString()) {
        QJsonObject p{{"task_id",taskId},{"cust_name",custName}};
        if (!fileName.isEmpty()) p["file_name"] = fileName;
        send({{"cmd","START"},{"params", p}});
    }
    // 停止录制
    void stopRecord() { send({{"cmd","STOP"},{"params",QJsonObject{}}}); }
    // 播放视频（弹出独立播放器）
    void playVideo(const QString &path) {
        send({{"cmd","PLAY"},{"params",QJsonObject{{"file_path",path}}}});
    }
private slots:
    void onRead() {
        m_buf += m_sock->readAll();
        while (m_buf.size() >= 4) {
            quint32 len = ((quint8)m_buf[0]<<24)|((quint8)m_buf[1]<<16)|
                          ((quint8)m_buf[2]<<8) |((quint8)m_buf[3]);
            if ((quint32)m_buf.size() < 4+len) break;
            QJsonObject resp = QJsonDocument::fromJson(m_buf.mid(4,len)).object();
            m_buf.remove(0, 4+len);
            onResponse(resp);
        }
    }
    void onResponse(const QJsonObject &r) {
        qDebug() << "收到响应:" << r["msg"].toString();
        // 处理主动事件
        if (r["type"].toString() == "event") {
            QString evt = r["event"].toString();
            if (evt == "RECORD_STOPPED") {
                QString filePath = r["data"].toObject()["file_path"].toString();
                qDebug() << "录制完成，文件:" << filePath;
            }
        }
    }
};
```
