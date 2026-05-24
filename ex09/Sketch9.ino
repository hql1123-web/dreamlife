#include <WiFi.h>
#include <WebServer.h>

// ========== WiFi 配置 (STA 模式) ==========
const char* ssid = "iQOO Neo8 Pro";      // 修改为你的 WiFi 名称
const char* password = "hdrthitegogsru5786@!";  // 修改为你的 WiFi 密码

WebServer server(80);

// ========== 硬件引脚 ==========
const int touchPin = T0;   // 触摸传感器引脚 (GPIO4)

// ========== 初始化 ==========
void setup() {
    Serial.begin(115200);

    // 连接 WiFi
    WiFi.begin(ssid, password);
    Serial.print("正在连接 WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi 已连接");
    Serial.print("IP 地址: ");
    Serial.println(WiFi.localIP());

    // HTTP 路由
    server.on("/", handleRoot);        // 主页面
    server.on("/data", handleData);    // 传感器数据接口
    server.begin();
    Serial.println("🌐 HTTP 服务器已启动");
}

void loop() {
    server.handleClient();   // 处理客户端请求
}

// ========== 主页面：实时仪表盘 ==========
void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>触摸传感器实时仪表盘</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Arial, sans-serif;
      background: radial-gradient(circle at center, #1a1a2e 0%, #0f0f1a 100%);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      flex-direction: column;
    }
    .dashboard {
      text-align: center;
      background: rgba(255,255,255,0.05);
      backdrop-filter: blur(10px);
      border-radius: 30px;
      padding: 50px 40px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.5);
      border: 1px solid rgba(255,255,255,0.1);
    }
    h1 {
      color: #aaa;
      font-weight: 300;
      font-size: 1.5em;
      letter-spacing: 3px;
      margin-bottom: 30px;
      text-transform: uppercase;
    }
    .value-container {
      position: relative;
      margin: 20px 0;
    }
    #touchValue {
      font-size: 8em;
      font-weight: bold;
      color: #4CAF50;
      text-shadow: 0 0 30px rgba(76, 175, 80, 0.6);
      line-height: 1;
    }
    .unit {
      font-size: 0.3em;
      color: #666;
      display: block;
    }
    .bar-container {
      width: 100%;
      height: 8px;
      background: rgba(255,255,255,0.1);
      border-radius: 4px;
      margin: 20px 0;
      overflow: hidden;
    }
    .bar-fill {
      height: 100%;
      width: 0%;
      background: linear-gradient(90deg, #4CAF50, #ff9800);
      border-radius: 4px;
      transition: width 0.1s ease;
    }
    .status {
      color: #888;
      font-size: 0.9em;
      margin-top: 15px;
    }
  </style>
</head>
<body>
  <div class="dashboard">
    <h1>触摸传感器实时数据</h1>
    <div class="value-container">
      <div id="touchValue">--</div>
      <span class="unit">原始模拟值 (0 ~ 100+)</span>
    </div>
    <div class="bar-container">
      <div class="bar-fill" id="barFill"></div>
    </div>
    <div class="status" id="status">等待数据...</div>
  </div>

  <script>
    // 定时拉取传感器数据（每 200ms）
    function fetchData() {
      fetch('/data')
        .then(response => {
          if (!response.ok) throw new Error('网络错误');
          return response.json();
        })
        .then(data => {
          // 更新数字
          document.getElementById('touchValue').textContent = data.value;
          // 更新进度条（触摸时数值减小，为了直观，将 0~100 映射为 100%~0%）
          // 原始值越大表示未触摸，越小表示触摸，这里做一个反向映射让视觉更直观
          let raw = data.value;
          let percent = Math.max(0, Math.min(100, 100 - raw));  // 假设原始值通常在 0~100 之间
          document.getElementById('barFill').style.width = percent + '%';
          // 更新状态文字
          document.getElementById('status').textContent = 
            `更新时间: ${new Date().toLocaleTimeString()}  |  数值: ${raw}`;
        })
        .catch(err => {
          document.getElementById('status').textContent = '⚠️ 获取数据失败，正在重试...';
          console.error(err);
        });
    }

    // 立即获取一次，然后每 200ms 刷新
    fetchData();
    setInterval(fetchData, 200);
  </script>
</body>
</html>
)rawliteral";
    server.send(200, "text/html; charset=utf-8", html);
}

// ========== 传感器数据接口（返回 JSON） ==========
void handleData() {
    int touchValue = touchRead(touchPin);   // 直接读取原始值
    String json = "{\"value\":" + String(touchValue) + "}";
    server.send(200, "application/json", json);
}