/*
 * 实验1：ESP32 PWM 呼吸灯
 * 开发板自带 LED 通常连接在 GPIO2
 * 效果：LED 缓慢变亮 → 缓慢变暗 → 循环往复
 * 串口输出呼吸周期计数（波特率 115200）
 */

const int ledPin = 2;          // 内置 LED 引脚（大多数 ESP32 开发板为 GPIO2）
const int pwmChannel = 0;      // LEDC 通道（0~15 任选）
const int freq = 5000;         // PWM 频率（Hz）
const int resolution = 8;      // 分辨率（8 位：0~255）
const int delayMs = 1;        // 每一步的延时（ms），可通过修改此数值改变呼吸速度

int cycleCount = 0;            // 呼吸周期计数器

void setup() {
    Serial.begin(115200);        // 初始化串口
    // 配置 LEDC 通道，将 ledPin 连接到该通道
    ledcSetup(pwmChannel, freq, resolution);
    ledcAttachPin(ledPin, pwmChannel);

    Serial.println("呼吸灯实验开始...");
}

void loop() {
    // 一个完整的呼吸周期：逐渐变亮 → 逐渐变暗
    // 变亮阶段：占空比从 0 增加到 255
    for (int duty = 0; duty <= 255; duty++) {
        ledcWrite(pwmChannel, duty);
        delay(delayMs);
    }
    // 变暗阶段：占空比从 255 减小到 0
    for (int duty = 255; duty >= 0; duty--) {
        ledcWrite(pwmChannel, duty);
        delay(delayMs);
    }

    // 完成一个完整的呼吸周期
    cycleCount++;
    Serial.print("呼吸周期计数：");
    Serial.println(cycleCount);
}