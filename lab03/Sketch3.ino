/*
 * 实验3：ESP32 PWM 呼吸灯
 * 开发板自带 LED 通常连接在 GPIO2
 * 效果：LED 缓慢变亮 → 缓慢变暗 → 循环往复
 * 串口输出呼吸周期计数（波特率 115200）
 */
 /*
  * 双 LED 呼吸灯（内置 + 外接）
  * 内置 LED：GPIO2
  * 外接 LED：GPIO5（需串联 220Ω 电阻）
  */

  // 引脚定义
const int builtInLed = 2;   // 内置 LED（大部分 ESP32 开发板为 GPIO2）
const int externalLed = 5;  // 外接 LED（可改为你实际连接的引脚）

// PWM 配置
const int freq = 5000;
const int resolution = 8;   // 8 位分辨率（0~255）
const int delayMs = 10;     // 呼吸速度，可修改

// 使用独立的 LEDC 通道
const int channelBuiltIn = 0;   // 通道 0 驱动内置 LED
const int channelExternal = 1;  // 通道 1 驱动外接 LED

int cycleCount = 0;

void setup() {
    Serial.begin(115200);

    // 配置两个 PWM 通道并绑定引脚
    ledcSetup(channelBuiltIn, freq, resolution);
    ledcAttachPin(builtInLed, channelBuiltIn);

    ledcSetup(channelExternal, freq, resolution);
    ledcAttachPin(externalLed, channelExternal);

    Serial.println("双 LED 呼吸灯实验开始...");
}

void loop() {
    // 一起变亮
    for (int duty = 0; duty <= 255; duty++) {
        ledcWrite(channelBuiltIn, duty);
        ledcWrite(channelExternal, duty);
        delay(delayMs);
    }
    // 一起变暗
    for (int duty = 255; duty >= 0; duty--) {
        ledcWrite(channelBuiltIn, duty);
        ledcWrite(channelExternal, duty);
        delay(delayMs);
    }

    cycleCount++;
    Serial.print("呼吸周期计数：");
    Serial.println(cycleCount);
}