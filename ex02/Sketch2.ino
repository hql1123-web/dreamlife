const int leds[] = { 2, 4, 5, 18 };  // 所有LED引脚
const int numLeds = 4;

void allLeds(int state) {
    for (int i = 0; i < numLeds; i++) {
        digitalWrite(leds[i], state);
    }
}

void setup() {
    Serial.begin(115200);
    for (int i = 0; i < numLeds; i++) {
        pinMode(leds[i], OUTPUT);
    }
    allLeds(LOW);
}

void loop() {
    // S: 三短
    for (int i = 0; i < 3; i++) {
        allLeds(HIGH); delay(200);
        allLeds(LOW);  delay(200);
    }
    delay(300);

    // O: 三长
    for (int i = 0; i < 3; i++) {
        allLeds(HIGH); delay(600);
        allLeds(LOW);  delay(200);
    }
    delay(300);

    // S: 三短
    for (int i = 0; i < 3; i++) {
        allLeds(HIGH); delay(200);
        allLeds(LOW);  delay(200);
    }
    delay(2000);
}