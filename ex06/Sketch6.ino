/*
 * ESP32 警车双闪灯效（双通道 PWM 反相呼吸灯）
 * 功能：两个 LED 呈现平滑交替渐变，一个渐亮时另一个渐暗，循环往复。
 * 硬件连接：
 *   - LED_A 阳极 -> 220Ω电阻 -> GPIO5
 *   - LED_A 阴极 -> GND
 *   - LED_B 阳极 -> 220Ω电阻 -> GPIO18
 *   - LED_B 阴极 -> GND
 * 说明：采用非阻塞方式，可轻松集成其他功能。
 */

const int ledPinA = 5;    // LED A 引脚
const int ledPinB = 18;   // LED B 引脚

// PWM 参数
const int freq = 5000;         // PWM 频率
const int resolution = 8;      // 分辨率 8 位（0-255）
const int channelA = 0;        // LED A 使用通道 0
const int channelB = 1;        // LED B 使用通道 1

int brightness = 0;            // A 灯的亮度（B 灯为 255 - brightness）
int fadeDirection = 1;         // 变化方向：1 为 A 渐亮（B 渐暗），-1 为 A 渐暗（B 渐亮）
unsigned long lastUpdate = 0;
const int updateInterval = 10; // 亮度更新间隔（毫秒），值越小过渡越快

void setup() {
    Serial.begin(115200);

    // 配置 PWM 通道并关联引脚
    ledcSetup(channelA, freq, resolution);
    ledcAttachPin(ledPinA, channelA);
    ledcSetup(channelB, freq, resolution);
    ledcAttachPin(ledPinB, channelB);

    // 初始状态：A 最暗，B 最亮
    ledcWrite(channelA, brightness);
    ledcWrite(channelB, 255 - brightness);

    Serial.println("警车双闪灯效启动（双 LED 反相呼吸）");
}

void loop() {
    // 非阻塞更新亮度
    if (millis() - lastUpdate >= updateInterval) {
        lastUpdate = millis();

        // 更新亮度值
        brightness += fadeDirection;
        if (brightness >= 255) {
            brightness = 255;
            fadeDirection = -1;   // 到达最亮后开始变暗
        }
        else if (brightness <= 0) {
            brightness = 0;
            fadeDirection = 1;    // 到达最暗后开始变亮
        }

        // 写入反相亮度
        ledcWrite(channelA, brightness);
        ledcWrite(channelB, 255 - brightness);
    }

    // 可选：每秒输出一次调试信息（可注释）
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 1000) {
        lastPrint = millis();
        Serial.printf("亮度: A=%d, B=%d\n", brightness, 255 - brightness);
    }
}