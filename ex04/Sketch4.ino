/*
 * ESP32 触摸自锁开关（外接LED版）
 * 功能：每检测到一次有效触摸（按下瞬间），翻转外接LED状态并保持。
 * 防抖：触摸状态需稳定 50ms 后才认为变化有效。
 * 硬件连接：
 *   - 触摸引脚 T0 (GPIO4) 接触摸电极（导线/铜箔）
 *   - 外接LED阳极 -> 220Ω电阻 -> GPIO5
 *   - 外接LED阴极 -> GND
 */

const int touchPin = T0;          // 触摸引脚，使用 ESP32 的 T0（对应 GPIO4）
const int ledPin = 5;             // 外接LED控制引脚（可改为其他可用GPIO）

bool ledState = false;            // LED 状态变量，初始熄灭
bool lastStableTouch = false;     // 上一次稳定后的触摸状态（true=正在触摸）
bool currentTouch = false;        // 当前原始触摸状态
bool lastRawTouch = false;        // 上一次原始读取值（用于检测变化）

unsigned long lastChangeTime = 0; // 上次原始状态发生变化的时间戳
const unsigned long debounceDelay = 50; // 防抖延时 (毫秒)

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, ledState ? HIGH : LOW);

    // 读取初始触摸状态，初始化防抖逻辑
    lastRawTouch = (touchRead(touchPin) < 20); // 阈值 20，可根据实际情况调整
    lastStableTouch = lastRawTouch;
    Serial.println("触摸自锁开关已启动（外接LED）");
    Serial.printf("初始 LED 状态: %s\n", ledState ? "亮" : "灭");
}

void loop() {
    // 1. 读取原始触摸值，并转换为布尔状态（低于阈值为触摸）
    int touchValue = touchRead(touchPin);
    currentTouch = (touchValue < 20); // 阈值 20：未触摸时数值较大（>40），触摸时较小

    // 2. 软件防抖：检测原始状态是否发生变化
    if (currentTouch != lastRawTouch) {
        // 状态改变，记录时间戳
        lastChangeTime = millis();
        lastRawTouch = currentTouch;
    }

    // 3. 如果状态改变后已经稳定超过防抖延时，则更新稳定状态
    if ((millis() - lastChangeTime) >= debounceDelay) {
        // 只有当稳定后的状态与上一次稳定状态不同时，才认为是真正的状态跃迁
        if (currentTouch != lastStableTouch) {
            lastStableTouch = currentTouch;

            // 4. 边缘检测：只处理“从未触摸到触摸”的上升沿
            if (lastStableTouch == true) {
                // 检测到一次有效按下（上升沿），翻转 LED 状态
                ledState = !ledState;
                digitalWrite(ledPin, ledState ? HIGH : LOW);
                Serial.printf("触摸事件触发！LED 状态变为: %s\n", ledState ? "亮" : "灭");
            }
        }
    }

    // 可选：每秒输出一次调试信息（方便观察数值）
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime >= 1000) {
        lastPrintTime = millis();
        Serial.printf("触摸值: %d, 当前触摸状态: %d, LED: %d\n",
            touchValue, currentTouch, ledState);
    }
}