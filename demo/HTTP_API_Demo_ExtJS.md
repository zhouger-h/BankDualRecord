# 银行柜面双录控件 — HTTP API 调用示例（Ext JS）

> 服务地址：`http://127.0.0.1:9528`（默认端口，可在配置界面修改）

---

## 一、基础配置

```javascript
// 全局配置 HTTP 服务基础地址
Ext.Ajax.setDefaultHeaders({
    'Content-Type': 'application/json; charset=UTF-8'
});

var BASE_URL = 'http://127.0.0.1:9528';
```

---

## 二、心跳检测（PING）

```javascript
Ext.Ajax.request({
    url: BASE_URL + '/ping',
    method: 'GET',
    timeout: 5000,
    success: function(response) {
        var result = Ext.decode(response.responseText);
        // result: {"code":0,"msg":"pong","ts":1713600000000}
        console.log('服务正常，时间戳：' + result.ts);
    },
    failure: function(response) {
        Ext.Msg.alert('错误', '双录服务不可用');
    }
});
```

---

## 三、查询录制状态（STATUS）

```javascript
Ext.Ajax.request({
    url: BASE_URL + '/status',
    method: 'GET',
    success: function(response) {
        var result = Ext.decode(response.responseText);
        // result: {"code":0,"msg":"OK","data":{"state":"IDLE","file_path":""}}
        var state = result.data.state;
        console.log('当前状态：' + state);
        // state 可选值: IDLE / RECORDING / STOPPED
    }
});
```

---

## 四、获取视频列表（LIST）

```javascript
Ext.Ajax.request({
    url: BASE_URL + '/list',
    method: 'GET',
    success: function(response) {
        var result = Ext.decode(response.responseText);
        // result: {"code":0,"msg":"OK","list":[{...},{...}]}
        Ext.each(result.list, function(item) {
            console.log(Ext.String.format(
                '文件：{0}，大小：{1} 字节，修改时间：{2}',
                item.name, item.size, item.modified
            ));
        });
    }
});
```

---

## 五、通用命令入口（POST /cmd）

所有业务命令统一走 `POST /cmd`，通过 `cmd` 字段区分。

### 5.1 初始化配置（INIT）

```javascript
Ext.Ajax.request({
    url: BASE_URL + '/cmd',
    method: 'POST',
    jsonData: {
        cmd: 'INIT',
        params: {
            video_cache_path: '/home/user/BankDualRecord/videos',
            ftp_host: '192.168.1.100',
            ftp_port: 21,
            ftp_user: 'ftpuser',
            ftp_pass: 'ftppass',
            ftp_remote_dir: '/dualrecord/',
            upload_protocol: 'ftp',
            bandwidth_limit_kb: 500
        }
    },
    success: function(response) {
        var result = Ext.decode(response.responseText);
        // result: {"code":0,"msg":"初始化成功"}
        console.log(result.msg);
    }
});
```

### 5.2 显示主界面（SHOW_WINDOW）

```javascript
Ext.Ajax.request({
    url: BASE_URL + '/cmd',
    method: 'POST',
    jsonData: {
        cmd: 'SHOW_WINDOW',
        params: {
            file_name: ''    // 可选：预设录制文件名
        }
    },
    success: function(response) {
        var result = Ext.decode(response.responseText);
        // result: {"code":0,"msg":"主界面已显示","file_name":""}
        console.log(result.msg);
    }
});
```

### 5.3 开始录制（START）

```javascript
Ext.Ajax.request({
    url: BASE_URL + '/cmd',
    method: 'POST',
    jsonData: {
        cmd: 'START',
        params: {
            task_id: 'T20260420001',       // 任务编号（必填）
            cust_name: '张三',              // 客户姓名
            file_name: ''                   // 可选：指定录制文件名
        }
    },
    success: function(response) {
        var result = Ext.decode(response.responseText);
        // result: {"code":0,"msg":"录制已启动","task_id":"T20260420001","file_name":""}
        if (result.code === 0) {
            console.log('录制已启动，任务号：' + result.task_id);
        } else {
            Ext.Msg.alert('提示', '启动失败：' + result.msg);
        }
    },
    failure: function(response) {
        Ext.Msg.alert('错误', '请求失败，请检查服务是否运行');
    }
});
```

### 5.4 停止录制（STOP）

```javascript
Ext.Ajax.request({
    url: BASE_URL + '/cmd',
    method: 'POST',
    jsonData: {
        cmd: 'STOP',
        params: {}
    },
    success: function(response) {
        var result = Ext.decode(response.responseText);
        // result: {"code":0,"msg":"录制已停止","file_path":"/home/.../T20260420001.mp4"}
        console.log('录制已停止，文件：' + result.file_path);
    }
});
```

### 5.5 隐藏主界面（HIDE_WINDOW）

```javascript
Ext.Ajax.request({
    url: BASE_URL + '/cmd',
    method: 'POST',
    jsonData: {
        cmd: 'HIDE_WINDOW',
        params: {}
    },
    success: function(response) {
        var result = Ext.decode(response.responseText);
        // result: {"code":0,"msg":"主界面已隐藏"}
        console.log(result.msg);
    }
});
```

### 5.6 播放视频（PLAY）

```javascript
Ext.Ajax.request({
    url: BASE_URL + '/cmd',
    method: 'POST',
    jsonData: {
        cmd: 'PLAY',
        params: {
            file_path: '/home/user/BankDualRecord/videos/T20260420001.mp4'
        }
    },
    success: function(response) {
        var result = Ext.decode(response.responseText);
        // result: {"code":0,"msg":"播放器已启动","file_path":"..."}
        console.log(result.msg);
    }
});
```

### 5.7 删除视频（DELETE）

```javascript
Ext.Ajax.request({
    url: BASE_URL + '/cmd',
    method: 'POST',
    jsonData: {
        cmd: 'DELETE',
        params: {
            file_path: '/home/user/BankDualRecord/videos/T20260420001.mp4'
        }
    },
    success: function(response) {
        var result = Ext.decode(response.responseText);
        if (result.code === 0) {
            console.log('文件已删除');
        } else {
            Ext.Msg.alert('提示', '删除失败：' + result.msg);
        }
    }
});
```

---

## 六、SSE 事件订阅（实时录制状态推送）

Ext JS 没有 SSE 内置支持，使用原生 `EventSource` 监听，再回调 Ext 方法更新界面：

```javascript
// 建立 SSE 长连接
var eventSource = new EventSource(BASE_URL + '/events');

eventSource.onmessage = function(event) {
    var result = Ext.decode(event.data);
    // result: {"event":"RECORD_STARTED","data":{"task_id":"T001","file_path":"..."}}

    switch (result.event) {
        case 'RECORD_STARTED':
            console.log('录制开始，任务：' + result.data.task_id);
            // 更新界面状态
            Ext.getCmp('recordStatus').setText('录制中...');
            break;

        case 'RECORD_STOPPED':
            console.log('录制结束，文件：' + result.data.file_path);
            Ext.getCmp('recordStatus').setText('空闲');
            // 刷新视频列表
            loadVideoList();
            break;

        case 'RECORD_ERROR':
            console.error('录制异常：' + result.data.error);
            Ext.Msg.alert('录制异常', result.data.error);
            Ext.getCmp('recordStatus').setText('异常');
            break;

        case 'UPLOAD_QUEUED':
            console.log('文件已加入上传队列：' + result.data.file_path);
            break;

        case 'UPLOAD_SUCCESS':
            console.log('上传完成：' + result.data.file_path);
            Ext.Msg.notify('上传完成', result.data.file_path);
            // 刷新视频列表（上传成功后本地文件可能已删除）
            loadVideoList();
            break;

        case 'UPLOAD_FAILED':
            console.error('上传失败：' + result.data.file_path + '，原因：' + result.data.error);
            Ext.Msg.alert('上传失败', result.data.error);
            break;

        case 'WINDOW_SHOWN':
            console.log('主界面已显示');
            break;

        case 'WINDOW_HIDDEN':
            console.log('主界面已隐藏');
            break;
    }
};

eventSource.onerror = function() {
    console.warn('SSE 连接断开，5秒后重连...');
    // EventSource 会自动重连，此处仅做日志记录
};

// 关闭连接（页面卸载时）
// eventSource.close();
```

---

## 七、完整业务流程示例

典型柜面业务场景：显示界面 → 初始化 → 开始录制 → 停止录制 → 隐藏界面

```javascript
/**
 * 完整双录流程
 * @param {String} taskId    任务编号
 * @param {String} custName  客户姓名
 */
function startDualRecord(taskId, custName) {

    // 第1步：显示录制界面
    Ext.Ajax.request({
        url: BASE_URL + '/cmd',
        method: 'POST',
        jsonData: { cmd: 'SHOW_WINDOW', params: {} },
        success: function(response) {
            var result = Ext.decode(response.responseText);
            if (result.code !== 0) {
                Ext.Msg.alert('错误', result.msg);
                return;
            }

            // 第2步：开始录制
            Ext.Ajax.request({
                url: BASE_URL + '/cmd',
                method: 'POST',
                jsonData: {
                    cmd: 'START',
                    params: {
                        task_id: taskId,
                        cust_name: custName
                    }
                },
                success: function(response) {
                    var result = Ext.decode(response.responseText);
                    if (result.code === 0) {
                        Ext.Msg.notify('提示', '双录已开始（' + taskId + '）');
                    } else {
                        Ext.Msg.alert('错误', result.msg);
                    }
                }
            });
        }
    });
}

/**
 * 结束双录流程
 */
function stopDualRecord() {
    Ext.Ajax.request({
        url: BASE_URL + '/cmd',
        method: 'POST',
        jsonData: { cmd: 'STOP', params: {} },
        success: function(response) {
            var result = Ext.decode(response.responseText);
            console.log('录制已停止，文件：' + result.file_path);

            // 延迟1秒后隐藏界面（等待文件写入完成）
            Ext.defer(function() {
                Ext.Ajax.request({
                    url: BASE_URL + '/cmd',
                    method: 'POST',
                    jsonData: { cmd: 'HIDE_WINDOW', params: {} },
                    success: function() {
                        Ext.Msg.notify('提示', '双录已完成，界面已隐藏');
                    }
                });
            }, 1000);
        }
    });
}
```

---

## 八、Ext JS ViewModel + Store 集成示例

### 8.1 封装通用请求方法

```javascript
// DualRecord API 工具类
Ext.define('DualRecord.Api', {
    singleton: true,

    base_url: 'http://127.0.0.1:9528',

    /** 发送命令 */
    sendCommand: function(cmd, params, successFn, failureFn) {
        Ext.Ajax.request({
            url: this.base_url + '/cmd',
            method: 'POST',
            jsonData: { cmd: cmd, params: params || {} },
            success: function(response) {
                var result = Ext.decode(response.responseText);
                if (Ext.isFunction(successFn)) successFn(result);
            },
            failure: function(response) {
                var msg = '请求失败';
                try { msg = Ext.decode(response.responseText).msg; } catch(e) {}
                if (Ext.isFunction(failureFn)) failureFn(msg);
                else Ext.Msg.alert('错误', msg);
            }
        });
    },

    /** 快捷 GET 请求 */
    get: function(path, successFn) {
        Ext.Ajax.request({
            url: this.base_url + path,
            method: 'GET',
            success: function(response) {
                var result = Ext.decode(response.responseText);
                if (Ext.isFunction(successFn)) successFn(result);
            }
        });
    },

    // ── 快捷方法 ──
    ping: function(fn)  { this.get('/ping', fn); },
    status: function(fn){ this.get('/status', fn); },
    list: function(fn)  { this.get('/list', fn); },
    show: function(fn)  { this.sendCommand('SHOW_WINDOW', {}, fn); },
    hide: function(fn)  { this.sendCommand('HIDE_WINDOW', {}, fn); },
    start: function(taskId, custName, fn) {
        this.sendCommand('START', { task_id: taskId, cust_name: custName }, fn);
    },
    stop: function(fn)  { this.sendCommand('STOP', {}, fn); },
    init: function(cfg, fn) {
        this.sendCommand('INIT', cfg, fn);
    }
});
```

### 8.2 使用示例

```javascript
// 一行代码调用
DualRecord.Api.ping(function(r) {
    console.log('pong: ' + r.ts);
});

DualRecord.Api.start('T001', '李四', function(r) {
    if (r.code === 0) Ext.Msg.notify('成功', '录制已开始');
});
```

---

## 九、接口一览表

| 方法 | 路径 | 说明 | 参数 |
|------|------|------|------|
| `POST` | `/cmd` | 通用命令入口 | `{"cmd":"命令名","params":{...}}` |
| `GET`  | `/ping` | 心跳检测 | — |
| `GET`  | `/status` | 查询录制状态 | — |
| `GET`  | `/list` | 获取视频列表 | — |
| `GET`  | `/events` | SSE 事件订阅 | — |

### 命令列表（cmd 字段）

| 命令 | 说明 | 主要参数 |
|------|------|----------|
| `INIT` | 初始化配置 | `video_cache_path`, `ftp_host`, `ftp_port`, `ftp_user`, `ftp_pass`, `ftp_remote_dir`, `upload_protocol`, `bandwidth_limit_kb` |
| `SHOW_WINDOW` | 显示录制界面 | `file_name`（可选） |
| `HIDE_WINDOW` | 隐藏录制界面 | — |
| `START` | 开始录制 | `task_id`（必填）, `cust_name`, `file_name`（可选） |
| `STOP` | 停止录制 | — |
| `PLAY` | 播放视频 | `file_path` |
| `DELETE` | 删除视频 | `file_path` |
| `STATUS` | 查询状态 | — |
| `LIST` | 视频列表 | — |
| `PING` | 心跳检测 | — |

### SSE 事件类型

| 事件 | 说明 | 附加数据 |
|------|------|----------|
| `RECORD_STARTED` | 录制开始 | `task_id`, `file_path` |
| `RECORD_STOPPED` | 录制结束 | `task_id`, `file_path` |
| `RECORD_ERROR` | 录制异常 | `task_id`, `error` |
| `UPLOAD_QUEUED` | 文件已加入上传队列 | `file_path` |
| `UPLOAD_SUCCESS` | 上传完成 | `file_path` |
| `UPLOAD_FAILED` | 上传失败 | `file_path`, `error` |
| `WINDOW_SHOWN` | 主界面已显示 | — |
| `WINDOW_HIDDEN` | 主界面已隐藏 | — |
