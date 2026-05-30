#include <WiFi.h>
#include <WebServer.h>

// ========== 1. 引脚定义 ==========
#define AIN1 4
#define AIN2 5
#define PWMA 6
#define BIN1 7
#define BIN2 8
#define PWMB 10
#define STBY 9

// ========== 2. 电机控制参数 ==========
int baseSpeed = 200;
String currentState = "stop";  // 记录当前状态：forward/backward/left/right/stop

// ========== 3. Wi-Fi 设置 ==========
const char* ssid = "MY_ESP32_C3";
const char* password = "12345678";
WebServer server(80);

// ====== 函数原型声明 ======
void forward(int speed);
void backward(int speed);
void turnLeft(int speed);
void turnRight(int speed);
void stopCar();
void refreshMotion();

void handleRoot();
void handleForward();
void handleBackward();
void handleLeft();
void handleRight();
void handleStop();
void handleSpeedUp();
void handleSpeedDown();
void handleSpeedVal();

void setup() {
    pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT); pinMode(PWMA, OUTPUT);
    pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT); pinMode(PWMB, OUTPUT);
    pinMode(STBY, OUTPUT);

    digitalWrite(STBY, HIGH);
    stopCar();

    WiFi.softAP(ssid, password);
    Serial.begin(115200);
    Serial.print("小车已启动，IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/forward", handleForward);
    server.on("/backward", handleBackward);
    server.on("/left", handleLeft);
    server.on("/right", handleRight);
    server.on("/stop", handleStop);
    server.on("/speed_up", handleSpeedUp);
    server.on("/speed_down", handleSpeedDown);
    server.on("/speed_val", handleSpeedVal);
    server.begin();
    Serial.println("Web服务器已启动");
}

void loop() {
    server.handleClient();
}

// ========== 4. 小车控制函数 ==========
void forward(int speed) {
    digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
    analogWrite(PWMA, speed); analogWrite(PWMB, speed);
}

void backward(int speed) {
    digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
    analogWrite(PWMA, speed); analogWrite(PWMB, speed);
}

void turnLeft(int speed) {
    digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
    analogWrite(PWMA, speed); analogWrite(PWMB, speed);
}

void turnRight(int speed) {
    digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
    analogWrite(PWMA, speed); analogWrite(PWMB, speed);
}

void stopCar() {
    digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH);
    analogWrite(PWMA, 0); analogWrite(PWMB, 0);
}

// 根据当前状态重新设置电机速度
void refreshMotion() {
    if (currentState == "forward")      forward(baseSpeed);
    else if (currentState == "backward") backward(baseSpeed);
    else if (currentState == "left")     turnLeft(baseSpeed);
    else if (currentState == "right")    turnRight(baseSpeed);
}

// ========== 5. Web请求处理函数 (已根据实测修正映射) ==========
void handleForward() {
    currentState = "forward";
    turnLeft(baseSpeed);   // 网页“前”实际执行左转
    server.send(200, "text/plain", "OK");
}

void handleBackward() {
    currentState = "backward";
    turnRight(baseSpeed);  // 网页“后”实际执行右转
    server.send(200, "text/plain", "OK");
}

void handleLeft() {
    currentState = "left";
    forward(baseSpeed);    // 网页“左”实际执行前进
    server.send(200, "text/plain", "OK");
}

void handleRight() {
    currentState = "right";
    backward(baseSpeed);   // 网页“右”实际执行后退
    server.send(200, "text/plain", "OK");
}

void handleStop() {
    currentState = "stop";
    stopCar();
    server.send(200, "text/plain", "OK");
}

void handleSpeedUp() {
    baseSpeed += 20;
    if (baseSpeed > 255) baseSpeed = 255;
    refreshMotion();
    server.send(200, "text/plain", String(baseSpeed));
}

void handleSpeedDown() {
    baseSpeed -= 20;
    if (baseSpeed < 80) baseSpeed = 80;
    refreshMotion();
    server.send(200, "text/plain", String(baseSpeed));
}

void handleSpeedVal() {
    server.send(200, "text/plain", String(baseSpeed));
}

// ========== 6. 主页 HTML ==========
void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32-C3 遥控车</title>
  <style>
    body { text-align:center; font-family:Arial; margin-top:20px; }
    button { 
      font-size:30px; padding:20px 40px; margin:10px; border:none; 
      border-radius:10px; width:120px; height:80px; touch-action: manipulation;
    }
    .fwd { background:#4CAF50; color:white; }
    .bwd { background:#f44336; color:white; }
    .left { background:#2196F3; color:white; }
    .right { background:#FF9800; color:white; }
    .stop { background:#9E9E9E; color:white; }
    .spd { background:#3F51B5; color:white; font-size:20px; height:50px; width:60px; }
    .row { display:flex; justify-content:center; align-items:center; }
    #speedVal { font-size:24px; margin:0 15px; }
  </style>
</head>
<body>
  <h1>ESP32-C3 小车控制</h1>
  <div>
    <div class="row"><button class="fwd" onclick="sendCmd('/forward')">▲ 前</button></div>
    <div class="row">
      <button class="left" onclick="sendCmd('/left')">◀ 左</button>
      <button class="stop" onclick="sendCmd('/stop')">■ 停</button>
      <button class="right" onclick="sendCmd('/right')">▶ 右</button>
    </div>
    <div class="row"><button class="bwd" onclick="sendCmd('/backward')">▼ 后</button></div>
  </div>
  <br>
  <div class="row">
    <button class="spd" onclick="sendCmd('/speed_down')">-</button>
    <span id="speedVal">200</span>
    <button class="spd" onclick="sendCmd('/speed_up')">+</button>
  </div>

  <script>
    function sendCmd(url) {
      fetch(url)
        .then(response => {
          if (url === '/speed_up' || url === '/speed_down') {
            return response.text();
          }
          return null;
        })
        .then(text => {
          if (text) {
            document.getElementById('speedVal').textContent = text;
          }
        })
        .catch(err => console.log('Error:', err));
    }
    fetch('/speed_val')
      .then(r => r.text())
      .then(v => document.getElementById('speedVal').textContent = v);
  </script>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
}