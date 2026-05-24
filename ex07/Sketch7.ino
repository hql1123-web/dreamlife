#include <WiFi.h>
#include <WebServer.h>

// AP 模式配置
const char* ap_ssid = "ESP32_Dimmer";
const char* ap_password = "12345678";

WebServer server(80);

// LED PWM 设置
const int ledPin = 5;
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

// 根页面：返回优化后的滑动条界面
void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 无极调光器</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, #1e1e2f, #2a2a40);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      flex-direction: column;
    }
    .card {
      background: rgba(255,255,255,0.1);
      backdrop-filter: blur(15px);
      border-radius: 25px;
      padding: 50px 40px;
      box-shadow: 0 20px 50px rgba(0,0,0,0.5);
      text-align: center;
      width: 90%;
      max-width: 500px;
    }
    h1 {
      color: #ffffff;
      margin-bottom: 40px;
      font-size: 2.2em;
      font-weight: 300;
      letter-spacing: 2px;
    }
    .slider-container {
      width: 100%;
      margin: 30px 0;
    }
    input[type=range] {
      -webkit-appearance: none;
      appearance: none;
      width: 100%;
      height: 20px;
      background: rgba(255,255,255,0.2);
      border-radius: 10px;
      outline: none;
      transition: background 0.2s;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 50px;
      height: 50px;
      background: #4CAF50;
      border-radius: 50%;
      cursor: pointer;
      box-shadow: 0 0 20px #4CAF50;
      border: 3px solid #fff;
      transition: transform 0.1s;
    }
    input[type=range]::-webkit-slider-thumb:hover {
      transform: scale(1.1);
    }
    .value-display {
      font-size: 4em;
      font-weight: bold;
      color: #4CAF50;
      text-shadow: 0 0 15px rgba(76, 175, 80, 0.5);
      margin: 20px 0;
    }
    .unit {
      font-size: 0.5em;
      color: #aaa;
    }
    .status {
      color: #aaa;
      margin-top: 20px;
      font-size: 0.9em;
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>无极调光器</h1>
    <div class="slider-container">
      <input type="range" id="brightness" min="0" max="255" value="0" step="1">
    </div>
    <div class="value-display">
      <span id="brightnessValue">0</span><span class="unit"> / 255</span>
    </div>
    <div class="status">AP 模式 · 连接热点后访问 192.168.4.1</div>
  </div>

  <script>
    const slider = document.getElementById('brightness');
    const valueSpan = document.getElementById('brightnessValue');

    // 更新显示并发送请求
    function updateBrightness(value) {
      valueSpan.textContent = value;
      // 使用 fetch 发送 GET 请求（异步，不阻塞）
      fetch('/set?val=' + value)
        .catch(error => console.error('请求失败:', error));
    }

    // 监听 input 事件（拖动时连续触发）
    slider.addEventListener('input', function() {
      updateBrightness(this.value);
    });

    // 初始调用，同步状态
    updateBrightness(slider.value);
  </script>
</body>
</html>
)rawliteral";
    server.send(200, "text/html; charset=utf-8", html);
}

// 处理亮度调节请求
void handleSet() {
    if (server.hasArg("val")) {
        int brightness = constrain(server.arg("val").toInt(), 0, 255);
        ledcWrite(ledChannel, brightness);
        server.send(200, "text/plain", "OK");
        Serial.printf("亮度: %d\n", brightness);
    }
    else {
        server.send(400, "text/plain", "缺少参数");
    }
}

void setup() {
    Serial.begin(115200);

    // 初始化 PWM
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(ledPin, ledChannel);
    ledcWrite(ledChannel, 0);

    // 开启 AP 模式
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("📡 ESP32 热点已开启");
    Serial.print("   SSID: "); Serial.println(ap_ssid);
    Serial.print("   密码: "); Serial.println(ap_password);
    Serial.print("   访问地址: http://");
    Serial.println(WiFi.softAPIP());

    // 配置路由
    server.on("/", handleRoot);
    server.on("/set", handleSet);
    server.begin();
    Serial.println("🌐 HTTP 服务器已启动");
}

void loop() {
    server.handleClient();
}