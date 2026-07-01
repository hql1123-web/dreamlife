#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "heartRate.h"          // 使用成熟的心率检测算法

// ---------- 硬件引脚 ----------
#define INNER_PIN  4
#define OUTER_PIN  5
#define INNER_NUM  8
#define OUTER_NUM  16

#define OLED_ADDR  0x3C
#define SCREEN_W   128
#define SCREEN_H   64

// ---------- 灯环对象 ----------
Adafruit_NeoPixel inner(INNER_NUM, INNER_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel outer(OUTER_NUM, OUTER_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ---------- MAX30102 寄存器地址 ----------
#define MAX30102_ADDR  0x57
#define REG_FIFO_DATA  0x07

// ---------- 心率变量 ----------
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE], rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg = 0;

// ---------- 动画控制 ----------
bool beatFlag = false;
unsigned long beatStart = 0;
const int animDur = 800;

// ---------- 波形显示 ----------
#define WAVE_SIZE  100
uint32_t waveBuf[WAVE_SIZE];      // ★ 改为 uint32_t，匹配红外数据
int waveIdx = 0;
unsigned long lastDisp = 0;
const int dispInterval = 40;

// ---------- 手指检测与预热 ----------
#define FINGER_THRESHOLD  50000     // 红外值低于此视为无手指
int warmUpCnt = 0;
const int WARM_UP = 200;

// ---------- 传感器初始化（直接寄存器操作） ----------
void initMAX30102() {
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(0x09); Wire.write(0x40);   // 软复位
    Wire.endTransmission();
    delay(100);

    // 清零 FIFO 读写指针及溢出计数 (0x04~0x06)
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(0x04);
    Wire.write(0x00); Wire.write(0x00); Wire.write(0x00);
    Wire.write(0x00); Wire.write(0x00);
    Wire.endTransmission();

    // FIFO 配置 (0x08): 1次平均, 不覆盖, 几乎满中断使能
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(0x08); Wire.write(0x20);
    Wire.endTransmission();

    // 模式 (0x09): SpO2 模式 (红光+红外)
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(0x09); Wire.write(0x03);
    Wire.endTransmission();

    // SpO2 配置 (0x0A): ADC范围16384, 采样率400Hz, 脉宽411μs
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(0x0A); Wire.write(0x27);
    Wire.endTransmission();

    // LED 电流 (0x0C 红光, 0x0D 红外) 中等亮度
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(0x0C); Wire.write(0x1F);
    Wire.endTransmission();
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(0x0D); Wire.write(0x1F);
    Wire.endTransmission();

    // 再次清零指针
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(0x04);
    Wire.write(0x00); Wire.write(0x00); Wire.write(0x00);
    Wire.write(0x00); Wire.write(0x00);
    Wire.endTransmission();

    Serial.println("MAX30102 initialized.");
}

// ---------- 从 FIFO 读取一个红外采样值 ----------
uint32_t readFIFO() {
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(REG_FIFO_DATA);
    Wire.endTransmission(false);
    Wire.requestFrom(MAX30102_ADDR, 6);   // 每次采样包含 3字节红外 + 3字节红光

    if (Wire.available() >= 6) {
        uint8_t data[6];
        for (int i = 0; i < 6; i++) data[i] = Wire.read();
        // 红外通道通常在后3字节，高18位有效
        uint32_t ir = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5];
        return ir & 0x03FFFF;   // 截取18位
    }
    return 0;
}

// ---------- 双环白光涟漪动画 ----------
void animate() {
    unsigned long t = millis() - beatStart;
    if (t < 200) {
        int b = map(t, 0, 200, 0, 255);
        for (int i = 0; i < INNER_NUM; i++) inner.setPixelColor(i, inner.Color(b, b, b));
        inner.show();
        outer.clear(); outer.show();
    }
    else if (t < 500) {
        for (int i = 0; i < INNER_NUM; i++) inner.setPixelColor(i, inner.Color(255, 255, 255));
        inner.show();
        int b = map(t, 200, 500, 0, 255);
        for (int i = 0; i < OUTER_NUM; i++) outer.setPixelColor(i, outer.Color(b, b, b));
        outer.show();
    }
    else if (t < animDur) {
        int f = map(t, 500, animDur, 255, 0);
        for (int i = 0; i < INNER_NUM; i++) inner.setPixelColor(i, inner.Color(f, f, f));
        for (int i = 0; i < OUTER_NUM; i++) outer.setPixelColor(i, inner.Color(f, f, f));
        inner.show(); outer.show();
    }
    else {
        inner.clear(); inner.show();
        outer.clear(); outer.show();
        beatFlag = false;
    }
}

// ---------- OLED 波形绘制（★ 修正版） ----------
void drawWave() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("BPM: "); display.print(beatAvg);

    int gy = 12, gh = 50;
    display.drawLine(0, gy, SCREEN_W - 1, gy, SSD1306_WHITE);

    // 使用正确的初始值找最小值、最大值
    uint32_t minV = UINT32_MAX;
    uint32_t maxV = 0;
    for (int i = 0; i < WAVE_SIZE; i++) {
        if (waveBuf[i] < minV) minV = waveBuf[i];
        if (waveBuf[i] > maxV) maxV = waveBuf[i];
    }

    // 如果信号波动太小（<1000），人为扩展显示范围，避免波形成直线
    if (maxV - minV < 1000) {
        uint32_t mid = (maxV + minV) / 2;
        minV = (mid > 500) ? mid - 500 : 0;
        maxV = mid + 500;
    }

    int px = 0, py = 0;
    bool first = true;
    for (int i = 0; i < WAVE_SIZE; i++) {
        int idx = (waveIdx + i) % WAVE_SIZE;
        int x = map(i, 0, WAVE_SIZE - 1, 0, SCREEN_W - 1);
        int y = gy + gh - map(waveBuf[idx], minV, maxV, 0, gh);
        if (first) first = false;
        else display.drawLine(px, py, x, y, SSD1306_WHITE);
        px = x; py = y;
    }
    display.display();
}

// ===================== 初始化 =====================
void setup() {
    Serial.begin(115200);
    Wire.begin();

    inner.begin(); inner.clear(); inner.setBrightness(60); inner.show();
    outer.begin(); outer.clear(); outer.setBrightness(60); outer.show();

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("OLED init fail"); while (1);
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0); display.println("Heart Monitor");
    display.display();
    delay(1000);

    initMAX30102();

    for (int i = 0; i < WAVE_SIZE; i++) waveBuf[i] = 0;
    Serial.println("Place finger on sensor.");
}

// ===================== 主循环 =====================
void loop() {
    uint32_t ir = readFIFO();

    // 有效手指检测
    if (ir >= FINGER_THRESHOLD) {
        // 预热阶段：只累积数据，不进行心率检测
        if (warmUpCnt < WARM_UP) {
            warmUpCnt++;
            if (warmUpCnt == WARM_UP) {
                Serial.println("Warm-up done. HR detection active.");
            }
        }
        else {
            // 使用成熟的心跳检测算法
            if (checkForBeat(ir)) {
                long delta = millis() - lastBeat;
                lastBeat = millis();
                beatsPerMinute = 60.0 / (delta / 1000.0);
                if (beatsPerMinute > 20 && beatsPerMinute < 255) {
                    rates[rateSpot++] = (byte)beatsPerMinute;
                    rateSpot %= RATE_SIZE;
                    beatAvg = 0;
                    for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
                    beatAvg /= RATE_SIZE;
                }
                Serial.print("BPM: "); Serial.println(beatAvg);
                beatFlag = true;
                beatStart = millis();
            }
        }
        // 存储波形（含预热阶段的数据）
        waveBuf[waveIdx] = ir;
        waveIdx = (waveIdx + 1) % WAVE_SIZE;
    }

    // 定时刷新 OLED
    if (millis() - lastDisp > dispInterval) {
        lastDisp = millis();
        drawWave();
    }

    // 运行动画
    if (beatFlag) animate();

    delay(2);   // 400Hz 采样率需要 2.5ms 间隔，2ms 近似
}