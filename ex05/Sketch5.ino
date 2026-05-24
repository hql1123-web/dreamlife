/*
 * ESP32 多档位触摸调速呼吸灯（非阻塞版）
 * 功能：LED 持续呼吸，每触摸一次 T0 引脚，切换一档呼吸速度（共 3 档）。
 * 硬件连接：
 *   - 触摸引脚 T0 (GPIO4) 接触摸电极
 *   - 外接LED阳极 -> 220Ω电阻 -> GPIO5
 *   - 外接LED阴极 -> GND
 * 速度档位：
 *   1档：缓慢呼吸 (间隔 30ms)
 *   2档：中速呼吸 (间隔 15ms)
 *   3档：快速呼吸 (间隔 5ms)
 */

const int touchPin = T0;       // 触摸引脚
const int ledPin = 5;          // LED PWM 引脚 (可用其他 GPIO)

// PWM 参数
const int freq = 5000;         // PWM 频率
const int ledChannel = 0;      // LEDC 通道
const int resolution = 8;      // 分辨率 8 位（0-255）

// 速度档位对应的更新间隔（毫秒）
const int speedIntervals[3] = { 30, 15, 5 };
int speedLevel = 0;            // 当前档位索引 0,1,2 -> 1档,2档,3档

// 呼吸灯状态
int brightness = 0;            // 当前亮度 (0-255)
int fadeDirection = 1;         // 变化方向：1 为渐亮，-1 为渐暗
unsigned long lastBrightnessUpdate = 0; // 上次亮度更新时间

// 触摸状态变量（边缘检测 + 防抖）
bool lastStableTouch = false;  // 上一次稳定触摸状态
bool currentRawTouch = false;  // 当前原始触摸状态
bool lastRawTouch = false;     // 上一次原始状态
unsigned long lastChangeTime = 0;       // 原始状态改变时间
const unsigned long debounceDelay = 50; // 防抖延时 50ms

void setup() {
    Serial.begin(115200);

    // 配置 LED PWM
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(ledPin, ledChannel);
    ledcWrite(ledChannel, brightness); // 初始亮度 0

    // 初始化触摸状态
    lastRawTouch = (touchRead(touchPin) < 20);
    lastStableTouch = lastRawTouch;

    Serial.println("多档位触摸调速呼吸灯启动");
    Serial.println("触摸 T0 引脚切换速度：1档(慢) -> 2档(中) -> 3档(快) -> 循环");
    Serial.printf("当前速度档位: %d (间隔 %dms)\n", speedLevel + 1, speedIntervals[speedLevel]);
}

void loop() {
    // 1. 读取触摸并执行防抖与边缘检测
    int touchValue = touchRead(touchPin);
    currentRawTouch = (touchValue < 20); // 阈值 20，可根据实际情况调整

    if (currentRawTouch != lastRawTouch) {
        lastChangeTime = millis();
        lastRawTouch = currentRawTouch;
    }

    if ((millis() - lastChangeTime) >= debounceDelay) {
        if (currentRawTouch != lastStableTouch) {
            lastStableTouch = currentRawTouch;

            // 上升沿：从未触摸到触摸，切换速度档位
            if (lastStableTouch == true) {
                speedLevel = (speedLevel + 1) % 3; // 0->1->2->0 循环
                Serial.printf("触摸事件！切换至档位 %d (间隔 %dms)\n",
                    speedLevel + 1, speedIntervals[speedLevel]);
            }
        }
    }

    // 2. 呼吸灯更新（非阻塞）
    if (millis() - lastBrightnessUpdate >= speedIntervals[speedLevel]) {
        lastBrightnessUpdate = millis();

        // 更新亮度
        brightness += fadeDirection;
        if (brightness >= 255) {
            brightness = 255;
            fadeDirection = -1;      // 开始渐暗
        }
        else if (brightness <= 0) {
            brightness = 0;
            fadeDirection = 1;       // 开始渐亮
        }
        ledcWrite(ledChannel, brightness);
    }

    // 可选：每 2 秒输出一次状态（调试用，可注释掉）
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime >= 2000) {
        lastPrintTime = millis();
        Serial.printf("触摸值: %d, 档位: %d, 亮度: %d\n",
            touchValue, speedLevel + 1, brightness);
    }
}
