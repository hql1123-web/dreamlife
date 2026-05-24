#include <WiFi.h>
#include <WebServer.h>

// ========== WiFi 配置 (STA 模式) ==========
const char* ssid = "iQOO Neo8 Pro";
const char* password = "hdrthitegogsru5786@!";

WebServer server(80);

// ========== 硬件引脚 ==========
const int touchPin = T0;       // 触摸引脚 (GPIO4)
const int ledPin = 5;          // 报警 LED (外接，串联 220Ω 电阻)

// ========== 系统状态变量 ==========
bool armed = false;            // 是否布防
bool alarmActive = false;      // 是否报警中 （改名避免冲突）

// ========== 触摸防抖与边缘检测 ==========
bool lastStableTouch = false;
bool currentRaw = false;
bool lastRaw = false;
unsigned long lastChangeTime = 0;
const unsigned long debounceDelay = 50;

// ========== LED 闪烁控制 ==========
unsigned long lastBlinkTime = 0;
const int blinkInterval = 100; // 闪烁间隔 100ms
bool ledState = false;

// ========== HTML 页面 ==========
void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>安防报警器</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, #1a1a2e, #16213e);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    .panel {
      background: rgba(255,255,255,0.1);
      backdrop-filter: blur(15px);
      border-radius: 30px;
      padding: 40px 30px;
      width: 90%;
      max-width: 400px;
      text-align: center;
      box-shadow: 0 15px 40px rgba(0,0,0,0.6);
    }
    h1 {
      color: #ffffff;
      font-weight: 300;
      margin-bottom: 30px;
      font-size: 2em;
    }
    .status-circle {
      width: 80px;
      height: 80px;
      border-radius: 50%;
      margin: 0 auto 25px;
      background: #555;
      transition: background 0.3s;
      box-shadow: 0 0 20px rgba(255,255,255,0.2);
    }
    .status-circle.armed { background: #ff9800; box-shadow: 0 0 25px #ff9800; }
    .status-circle.alarm { background: #f44336; box-shadow: 0 0 30px #f44336; animation: pulse 0.5s infinite alternate; }
    @keyframes pulse { from { transform: scale(1); } to { transform: scale(1.1); } }
    .btn-group {
      display: flex;
      justify-content: space-around;
      margin: 25px 0;
    }
    button {
      font-size: 1.2em;
      padding: 15px 25px;
      border: none;
      border-radius: 15px;
      cursor: pointer;
      font-weight: bold;
      letter-spacing: 1px;
      transition: transform 0.1s, box-shadow 0.2s;
    }
    button:active { transform: scale(0.95); }
    .arm-btn { background: #4CAF50; color: white; }
    .disarm-btn { background: #f44336; color: white; }
    .status-text {
      color: #ccc;
      font-size: 1.1em;
      margin-top: 15px;
    }
  </style>
</head>
<body>
  <div class="panel">
    <h1>安防报警器</h1>
    <div class="status-circle" id="statusLed"></div>
    <div class="btn-group">
      <button class="arm-btn" onclick="sendCmd('arm')">🔒 布防</button>
      <button class="disarm-btn" onclick="sendCmd('disarm')">🔓 撤防</button>
    </div>
    <div class="status-text" id="statusText">系统待命</div>
  </div>

  <script>
    function updateUI(state) {
      const led = document.getElementById('statusLed');
      const text = document.getElementById('statusText');
      led.className = 'status-circle';
      if (state === 'alarm') {
        led.classList.add('alarm');
        text.textContent = '⚠️ 报警中！请撤防';
      } else if (state === 'armed') {
        led.classList.add('armed');
        text.textContent = '🔒 已布防，触摸触发报警';
      } else {
        text.textContent = '🔓 已撤防，安全';
      }
    }

    function sendCmd(cmd) {
      fetch('/' + cmd)
        .then(response => response.text())
        .then(state => updateUI(state))
        .catch(err => console.error(err));
    }

    // 页面加载时获取初始状态
    fetch('/status')
      .then(response => response.text())
      .then(state => updateUI(state));
  </script>
</body>
</html>
)rawliteral";
    server.send(200, "text/html; charset=utf-8", html);
}

// ========== 控制端点 ==========
void handleArm() {
    armed = true;
    alarmActive = false;        // 改名
    ledState = false;
    digitalWrite(ledPin, LOW);
    server.send(200, "text/plain", "armed");
    Serial.println("🔒 已布防");
}

void handleDisarm() {
    armed = false;
    alarmActive = false;
    ledState = false;
    digitalWrite(ledPin, LOW);
    server.send(200, "text/plain", "disarmed");
    Serial.println("🔓 已撤防");
}

void handleStatus() {
    if (alarmActive) server.send(200, "text/plain", "alarm");
    else if (armed) server.send(200, "text/plain", "armed");
    else server.send(200, "text/plain", "disarmed");
}

// ========== 初始化 ==========
void setup() {
    Serial.begin(115200);

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // 连接 WiFi (STA 模式)
    WiFi.begin(ssid, password);
    Serial.print("正在连接 WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi 已连接");
    Serial.print("IP 地址: ");
    Serial.println(WiFi.localIP());

    // 初始化触摸状态
    lastRaw = (touchRead(touchPin) < 20);
    lastStableTouch = lastRaw;

    // HTTP 路由
    server.on("/", handleRoot);
    server.on("/arm", handleArm);
    server.on("/disarm", handleDisarm);
    server.on("/status", handleStatus);
    server.begin();
    Serial.println("🌐 HTTP 服务器已启动");
}

void loop() {
    server.handleClient();

    // -------- 触摸检测（防抖 + 边缘检测）--------
    int touchValue = touchRead(touchPin);
    currentRaw = (touchValue < 20);

    if (currentRaw != lastRaw) {
        lastChangeTime = millis();
        lastRaw = currentRaw;
    }

    if ((millis() - lastChangeTime) >= debounceDelay) {
        if (currentRaw != lastStableTouch) {
            lastStableTouch = currentRaw;

            if (lastStableTouch == true) {
                Serial.println("👆 触摸检测");
                if (armed && !alarmActive) {
                    alarmActive = true;     // 改名
                    Serial.println("🚨 报警触发！");
                }
            }
        }
    }

    // -------- 报警 LED 闪烁（非阻塞）--------
    if (alarmActive) {
        if (millis() - lastBlinkTime >= blinkInterval) {
            lastBlinkTime = millis();
            ledState = !ledState;
            digitalWrite(ledPin, ledState ? HIGH : LOW);
        }
    }
}