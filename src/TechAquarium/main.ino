#define BLYNK_TEMPLATE_ID "TMPL3EIaZN6X8"
#define BLYNK_TEMPLATE_NAME "TechAquarium"
#define BLYNK_AUTH_TOKEN "QBcMqHWJiiemdsvQ4_JndkwEmwtMGP25"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>

// WiFi & Blynk Credentials
char ssid[] = "Unencrypted ";
char pass[] = "11111111";

// Pins
#define TRIG_PIN 13
#define ECHO_PIN 12
#define TEMP_PIN 4
#define BUZZER_PIN 32
#define RELAY1_PIN 26  // Refill
#define RELAY2_PIN 27  // Drain
#define TDS_PIN 35
#define SERVO_PIN 25
#define MAX_DEPTH_CM 21.0

// Libraries and Objects
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
Servo feederServo;
BlynkTimer timer;

// Variables
float distance = 0.0, temperature = 0.0, tdsValue = 0.0;
float waterLevelPercent = 0.0;

unsigned long previousFeedTime = 0;
unsigned long feedInterval = 300000UL;
unsigned long manualModeStartTime = 0;
unsigned long lastActivityTime = 0;
unsigned long lastBlynkSend = 0;
unsigned long oledPageSwitchTime = 0;

int tempThreshold = 35;
int tdsThreshold = 500;
int waterLevelThreshold = 50;

bool isDraining = false, isRefilling = false;
bool manualDrain = false, manualRefill = false;
bool buzzerAlert = false, buzzerAcknowledged = false;

enum Mode { AUTO, MANUAL };
Mode currentMode = AUTO;

int oledPage = 1;

// ---------- BLYNK HANDLERS ----------
BLYNK_WRITE(V4) {
  manualRefill = param.asInt();
  if (manualRefill) {
    currentMode = MANUAL;
    manualModeStartTime = millis();
    startRefill();
  } else stopRefill();
  resetActivityTimer();
}

BLYNK_WRITE(V5) {
  if (param.asInt()) {
    currentMode = MANUAL;
    manualModeStartTime = millis();
    feedFish();
    resetActivityTimer();
  }
}

BLYNK_WRITE(V6) {
  manualDrain = param.asInt();
  if (manualDrain) {
    currentMode = MANUAL;
    manualModeStartTime = millis();
    startDrain();
  } else stopDrain();
  resetActivityTimer();
}

BLYNK_WRITE(V7) { tempThreshold = param.asInt(); }
BLYNK_WRITE(V8) { tdsThreshold = param.asInt(); }
BLYNK_WRITE(V9) { feedInterval = param.asInt() * 1000UL; }

BLYNK_WRITE(V10) {
  if (param.asInt()) {
    buzzerAcknowledged = true;
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ---------- CONTROL HELPERS ----------
void resetActivityTimer() {
  lastActivityTime = millis();
}

void feedFish() {
  feederServo.write(90);
  timer.setTimeout(1000L, []() { feederServo.write(0); });
  previousFeedTime = millis();
  resetActivityTimer();
}

void startRefill() {
  digitalWrite(RELAY1_PIN, LOW);
  isRefilling = true;
  resetActivityTimer();
}

void stopRefill() {
  digitalWrite(RELAY1_PIN, HIGH);
  isRefilling = false;
  resetActivityTimer();
}

void startDrain() {
  digitalWrite(RELAY2_PIN, LOW);
  isDraining = true;
  resetActivityTimer();
}

void stopDrain() {
  digitalWrite(RELAY2_PIN, HIGH);
  isDraining = false;
  resetActivityTimer();
}

// ---------- SENSORS ----------
float getAverageDistance() {
  long sum = 0;
  for (int i = 0; i < 5; i++) {
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    sum += pulseIn(ECHO_PIN, HIGH, 30000);
    delay(10);
  }
  return (sum / 5.0) * 0.034 / 2.0;
}

float getStableTDS() {
  static int readings[7] = {0}, index = 0;
  static float smoothedTDS = 0;
  static unsigned long lastRead = 0;

  if (millis() - lastRead >= 1000) {
    readings[index] = analogRead(TDS_PIN);
    index = (index + 1) % 7;
    lastRead = millis();

    long total = 0;
    for (int i = 0; i < 7; i++) total += readings[i];
    int avgReading = total / 7;
    float newTDS = map(avgReading, 0, 4095, 0, 1000);

    if (abs(newTDS - smoothedTDS) > 1)
      smoothedTDS += (newTDS > smoothedTDS) ? 1 : -1;
    else
      smoothedTDS = newTDS;
  }
  return smoothedTDS;
}

// ---------- DISPLAY ----------
void displayOLED() {
  unsigned long now = millis();
  bool anyPumpOn = isRefilling || isDraining || manualRefill || manualDrain;
  bool anyAlert = (temperature > tempThreshold || tdsValue > tdsThreshold);
  bool isFeeding = (now - previousFeedTime < 5000);

  // OLED sleep time updated to 5 minutes (300000 ms)
  if (!anyPumpOn && !anyAlert && !isFeeding && now - lastActivityTime > 300000) {
    u8g2.clearDisplay();
    u8g2.setPowerSave(1);
    return;
  } else u8g2.setPowerSave(0);

  // OLED priority: feeding > pump > alert > status
  if (isFeeding) oledPage = 4;
  else if (anyPumpOn) oledPage = 3;
  else if (anyAlert) oledPage = 2;
  else if (now - oledPageSwitchTime > 10000) {
    oledPage = 1;
    oledPageSwitchTime = now;
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  if (WiFi.status() == WL_CONNECTED) u8g2.drawDisc(123, 4, 3);
  u8g2.setCursor(0, 10);
  u8g2.print("Mode: ");
  u8g2.print(currentMode == MANUAL ? 'M' : 'A');

  switch (oledPage) {
    case 1:
      u8g2.setCursor(0, 25); u8g2.printf("Temp: %.1f C", temperature);
      u8g2.setCursor(0, 40); u8g2.printf("TDS: %.0f ppm", tdsValue);
      u8g2.setCursor(0, 55); u8g2.printf("Water: %.0f %%", waterLevelPercent);
      break;

    case 2:
      u8g2.setCursor(0, 25);
      if (temperature > tempThreshold)
        u8g2.print("Alert: High Temp!");
      else if (tdsValue > tdsThreshold)
        u8g2.print("Alert: High TDS!");
      else
        u8g2.print("Status: Normal");
      break;

    case 3:
      if (isRefilling || manualRefill) {
        u8g2.setCursor(0, 25); u8g2.print("Refilling Water...");
        u8g2.setCursor(0, 40); u8g2.printf("Level: %.0f%%", waterLevelPercent);
      }
      if (isDraining || manualDrain) {
        u8g2.setCursor(0, 55); u8g2.print("Draining Water...");
        u8g2.setCursor(0, 70); u8g2.printf("TDS: %.0f ppm", tdsValue);
      }
      break;

    case 4:
      u8g2.setCursor(0, 25); u8g2.print("Feeding in Progress...");
      break;
  }
  u8g2.sendBuffer();
}

// ---------- LOGIC ----------
void sendDataToBlynk() {
  if (WiFi.status() != WL_CONNECTED) return; // Skip if not connected

  if (millis() - lastBlynkSend >= 2000) {
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, tdsValue);
    Blynk.virtualWrite(V2, waterLevelPercent);

    String alertMsg = "";
    buzzerAlert = false;

    if (temperature > tempThreshold) {
      alertMsg += "High Temp! ";
      buzzerAlert = true;
    }
    if (tdsValue > tdsThreshold) {
      alertMsg += "High TDS! ";
      buzzerAlert = true;
    }
    if (millis() - previousFeedTime < 5000) alertMsg += "Feeding... ";
    if (isRefilling || manualRefill) alertMsg += "Refilling... ";
    if (isDraining || manualDrain) alertMsg += "Draining... ";

    if (alertMsg == "") alertMsg = "All Normal";

    Blynk.virtualWrite(V3, alertMsg);
    digitalWrite(BUZZER_PIN, (buzzerAlert && !buzzerAcknowledged) ? HIGH : LOW);
    lastBlynkSend = millis();
  }
}

void checkWaterLevel() {
  distance = getAverageDistance();
  if (distance < 1) distance = MAX_DEPTH_CM;  // Invalid reading
  waterLevelPercent = 100.0 - (distance / MAX_DEPTH_CM * 100.0);
}

void checkSensors() {
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  tdsValue = getStableTDS();
  checkWaterLevel();
}

void autoControl() {
  if (currentMode == MANUAL) {
    // Check for manual mode timeout (3 minutes)
    if (millis() - manualModeStartTime > 180000) {
      currentMode = AUTO;
      manualRefill = manualDrain = false;
      stopRefill();
      stopDrain();
    }
    return;
  }

  // Automatic logic
  if (waterLevelPercent < waterLevelThreshold) startRefill();
  else stopRefill();

  if (tdsValue > tdsThreshold) startDrain();
  else stopDrain();
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  feederServo.attach(SERVO_PIN);
  feederServo.write(0);

  sensors.begin();
  u8g2.begin();

  // Use Blynk.config() to not block if WiFi fails
  Blynk.config(BLYNK_AUTH_TOKEN);
  WiFi.begin(ssid, pass);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(100);
  }

  timer.setInterval(1000L, []() {
    checkSensors();
    autoControl();
    sendDataToBlynk();
    displayOLED();
  });
}

// ---------- LOOP ----------
void loop() {
  timer.run();
  if (WiFi.status() == WL_CONNECTED) Blynk.run();
}
