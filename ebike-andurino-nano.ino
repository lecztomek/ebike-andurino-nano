#include <EEPROM.h>
#include "LCDIC2.h"
#include "ui.h"
#include "simpleHXLevel.h"

class VirtualLCD {
private:
  uint8_t screen[2][16];
  uint8_t glyphs[8][8];
  bool glyphDefined[8];
  int cursorCol = 0;
  int cursorRow = 0;

  void resetScreenBuffer() {
    for (int r = 0; r < 2; r++) {
      for (int c = 0; c < 16; c++) {
        screen[r][c] = ' ';
      }
    }
  }

  void resetGlyphs() {
    for (int g = 0; g < 8; g++) {
      glyphDefined[g] = false;
      for (int i = 0; i < 8; i++) {
        glyphs[g][i] = 0;
      }
    }
  }

  bool isPrintable(uint8_t cell) const {
    return cell >= 32 && cell <= 126;
  }

  bool isCustomGlyph(uint8_t cell) const {
    return cell < 8 && glyphDefined[cell];
  }

  bool isSolidBlock(uint8_t cell) const {
    return cell == 255;
  }

  const char* pairToBlock(bool top, bool bottom) const {
    if (top && bottom) return "█";
    if (top) return "▀";
    if (bottom) return "▄";
    return " ";
  }

  void printHorizontalBorder() const {
    Serial.print('+');
    for (int i = 0; i < 95; i++) Serial.print('-'); // 16*5 + 15 odstepow
    Serial.println('+');
  }

  void printCustomCellRow(uint8_t glyph, int rowPair) const {
    int r0 = rowPair * 2;
    int r1 = r0 + 1;

    for (int px = 0; px < 5; px++) {
      bool top = glyphs[glyph][r0] & (1 << (4 - px));
      bool bottom = glyphs[glyph][r1] & (1 << (4 - px));
      Serial.print(pairToBlock(top, bottom));
    }
  }

  void printSolidCellRow() const {
    Serial.print("█████");
  }

  void printTextCellRow(uint8_t cell, int rowPair) const {
    // zwykly znak tylko w jednej "srodkowej" linii komorki
    if (rowPair == 1) {
      Serial.print("  ");
      Serial.print((char)cell);
      Serial.print("  ");
    } else {
      Serial.print("     ");
    }
  }

  void printEmptyCellRow() const {
    Serial.print("     ");
  }

public:
  bool begin() {
    Serial.begin(9600);
    resetGlyphs();
    clear();
    return true;
  }

  bool clear() {
    resetScreenBuffer();
    cursorCol = 0;
    cursorRow = 0;
    refresh();
    return true;
  }

  void setCursor(bool cursorEnabled) {
  }

  bool setCursor(int col, int row) {
    if (col >= 0 && col < 16 && row >= 0 && row < 2) {
      cursorCol = col;
      cursorRow = row;
      return true;
    }
    return false;
  }

  bool setGlyph(uint8_t glyph, const uint8_t buffer[]) {
    if (glyph >= 8 || buffer == nullptr) return false;

    for (int i = 0; i < 8; i++) {
      glyphs[glyph][i] = buffer[i];
    }
    glyphDefined[glyph] = true;
    return true;
  }

  bool print(uint8_t glyph) {
    if (cursorCol < 16) {
      screen[cursorRow][cursorCol++] = glyph;
      refresh();
      return true;
    }
    return false;
  }

  void print(const char* str) {
    while (*str && cursorCol < 16) {
      screen[cursorRow][cursorCol++] = (uint8_t)(*str++);
    }
    refresh();
  }

  void print(String str) {
    print(str.c_str());
  }

  void print(int num) {
    print(String(num));
  }

  void refresh() {
    printHorizontalBorder();

    for (int lcdRow = 0; lcdRow < 2; lcdRow++) {
      for (int rowPair = 0; rowPair < 4; rowPair++) {
        Serial.print('|');

        for (int c = 0; c < 16; c++) {
          uint8_t cell = screen[lcdRow][c];

          if (isSolidBlock(cell)) {
            printSolidCellRow();
          } else if (isCustomGlyph(cell)) {
            printCustomCellRow(cell, rowPair);
          } else if (isPrintable(cell)) {
            printTextCellRow(cell, rowPair);
          } else {
            printEmptyCellRow();
          }

          if (c < 15) Serial.print(' ');
        }

        Serial.println('|');
      }
    }

    printHorizontalBorder();
    Serial.println();
  }
};

//VirtualLCD lcd;
LCDIC2 lcd(0x27, 16, 2);

const byte totalScreens = 7;
byte currentScreen = 0;
ScreenDisplayState displayState;

const uint8_t hxDtPin  = A1;
const uint8_t hxSckPin = A2;

SimpleHXLevel hx(hxDtPin, hxSckPin);

int hxLevel = 0;                  // 0..100
long hxTorque = 0;                // surowy torque z biblioteki
bool hxError = false;
const char* hxErrorText = "OK";

bool serialWalkPressed = false;

const int pasPin = 2;
const int assistUpPin = 3;
const int assistDownPin = 4;
const int setButtonPin = 5;
const int walkAssistPin = 6;
const int pwmPin = 9;  

int pwmIdleVoltInt = 80;  
int pwmMinVoltInt = 100;  
int pwmMaxVoltInt = 420;  
int supplyVoltageInt = 500;

const byte bufferSize = 20;                 // Number of samples in the sliding window
byte pulseBuffer[bufferSize];               // Circular buffer holding recent pulse states (1 = pulse, 0 = no pulse)
byte bufferIndex = 0;

unsigned int sampleInterval = 50;    // Sampling period in milliseconds
unsigned long lastSampleTime = 0;

volatile bool pulseDetected = false;        // Flag set by interrupt if a PAS pulse was detected


int pulsesPerRevolution = 8;  

byte walkingAssistPower = 10;     
unsigned long walkingDelay = 2000; 

bool walkingAssistActive = false;
unsigned long walkingPressStart = 0;

volatile unsigned long lastMillis = 0;
const unsigned long interval = 500; 

byte numAssistLevels = 7;  
byte currentAssistLevel = 0;      

volatile unsigned int pulseCount = 0;

unsigned int rpm = 0;               
unsigned int minRPM = 31;         
unsigned int targetAssist = 0;     
unsigned int assistPower = 0;      
unsigned long lastUpdate = 0;

unsigned long rampTime = 1000;  
const unsigned long updateInterval = 100; 

volatile unsigned long lastInterruptTime = 0;
unsigned long walkingAssistStart = 0;

static unsigned long lastDisplayUpdateTime = 0;  
unsigned long displayRefreshInterval = 400; 

// --- Settings Mode ---
bool inSettingsMode = false;
bool lastInSettingsMode = false;
byte currentSettingIndex = 0;
unsigned long lastSetButtonPress = 0;
const unsigned long longPressTime = 1500;
const byte totalSettings = 20;
// ----------------------

bool hxZeroingEnabled = false;

enum AssistAlgorithm : uint8_t {
  ASSIST_STANDARD = 0,
  ASSIST_TORQUE   = 1,
  ASSIST_HYBRID   = 2
};

uint8_t assistMode = ASSIST_STANDARD;

// --- Hybrid Mode ---
const int hybridCorrectionMax = 4;              // maksymalna korekta: base +/- 4%

byte hybridLightHX = 5;                         // lekko
byte hybridNeutralHX = 10;                      // normalnie
byte hybridHeavyHX = 15;                        // ciężko

const int hybridCorrectionUpStepPerUpdate = 1;    // dokładanie mocy: wolniej, +1% co updateInterval
const int hybridCorrectionDownStepPerUpdate = 2;  // odejmowanie mocy: szybciej, -2% co updateInterval


int hybridCorrection = 0;                       // aktualna korekta -4..+4
int hybridBasePower = 0;                        // baza narastająca według rampTime
// -------------------

// --- Torque Mode ---
byte torqueStartLevel1HX = 20;       // poziom 1 -> trzeba mocno nacisnąć
byte torqueStartMaxLevelHX = 5;      // poziom max -> lekki nacisk
byte torqueStopHysteresis = 2;       // stopTh = startTh - hysteresis
// -------------------

void saveSettingsToEEPROM() {
  EEPROM.put(0, pulsesPerRevolution);
  EEPROM.put(2, walkingAssistPower);
  EEPROM.put(3, walkingDelay);
  EEPROM.put(7, minRPM);
  EEPROM.put(9, rampTime);
  EEPROM.put(13, numAssistLevels);
  EEPROM.put(14, sampleInterval); 
  EEPROM.put(16, pwmMinVoltInt);
  EEPROM.put(18, pwmMaxVoltInt);
  EEPROM.put(20, pwmIdleVoltInt);
  EEPROM.put(22, supplyVoltageInt);
  EEPROM.put(24, currentScreen);
  EEPROM.put(25, hxZeroingEnabled);
  EEPROM.put(26, assistMode);
  EEPROM.put(27, hybridLightHX);
  EEPROM.put(28, hybridNeutralHX);
  EEPROM.put(29, hybridHeavyHX);
  EEPROM.put(30, torqueStartLevel1HX);
  EEPROM.put(31, torqueStartMaxLevelHX);
  EEPROM.put(32, torqueStopHysteresis);
}

void loadSettingsFromEEPROM() {
  EEPROM.get(0, pulsesPerRevolution);
  if (pulsesPerRevolution < 1 || pulsesPerRevolution > 32) pulsesPerRevolution = 8;

  EEPROM.get(2, walkingAssistPower);
  if (walkingAssistPower < 1 || walkingAssistPower > 100) walkingAssistPower = 10;

  EEPROM.get(3, walkingDelay);
  if (walkingDelay < 0 || walkingDelay > 10000) walkingDelay = 2000;

  EEPROM.get(7, minRPM);
  if (minRPM < 1 || minRPM > 150) minRPM = 31;

  EEPROM.get(9, rampTime);
  if (rampTime < 200 || rampTime > 5000) rampTime = 1000;

  EEPROM.get(13, numAssistLevels);
  if (numAssistLevels < 1 || numAssistLevels > 20) numAssistLevels = 7;

  EEPROM.get(14, sampleInterval);
  if (sampleInterval < 10 || sampleInterval > 500) sampleInterval = 50;

  EEPROM.get(16, pwmMinVoltInt);
  if (pwmMinVoltInt < 0 || pwmMinVoltInt > 500) pwmMinVoltInt = 100;

  EEPROM.get(18, pwmMaxVoltInt);
  if (pwmMaxVoltInt < 0 || pwmMaxVoltInt > 500) pwmMaxVoltInt = 420;

  EEPROM.get(20, pwmIdleVoltInt);
  if (pwmIdleVoltInt < 0 || pwmIdleVoltInt > 500) pwmIdleVoltInt = 80;

  EEPROM.get(22, supplyVoltageInt);
  if (supplyVoltageInt < 300 || supplyVoltageInt > 600) supplyVoltageInt = 500;

  EEPROM.get(24, currentScreen);
  if (currentScreen >= totalScreens) currentScreen = 0;

  EEPROM.get(25, hxZeroingEnabled);

  EEPROM.get(26, assistMode);
  if (assistMode > ASSIST_HYBRID) assistMode = ASSIST_STANDARD;

  EEPROM.get(27, hybridLightHX);
  EEPROM.get(28, hybridNeutralHX);
  EEPROM.get(29, hybridHeavyHX);

  if (hybridLightHX > 100) hybridLightHX = 5;
  if (hybridNeutralHX > 100) hybridNeutralHX = 15;
  if (hybridHeavyHX > 100) hybridHeavyHX = 35;

  // pilnujemy kolejności: light < neutral < heavy
  if (hybridLightHX >= hybridNeutralHX || hybridNeutralHX >= hybridHeavyHX) {
    hybridLightHX = 5;
    hybridNeutralHX = 10;
    hybridHeavyHX = 15;
  }

  EEPROM.get(30, torqueStartLevel1HX);
  EEPROM.get(31, torqueStartMaxLevelHX);
  EEPROM.get(32, torqueStopHysteresis);

  if (torqueStartLevel1HX < 1 || torqueStartLevel1HX > 100) {
    torqueStartLevel1HX = 20;
  }

  if (torqueStartMaxLevelHX < 1 || torqueStartMaxLevelHX > 100) {
    torqueStartMaxLevelHX = 5;
  }

  if (torqueStopHysteresis < 1 || torqueStopHysteresis > 20) {
    torqueStopHysteresis = 2;
  }

  // pilnujemy, żeby poziom 1 miał wyższy próg niż poziom max
  if (torqueStartLevel1HX <= torqueStartMaxLevelHX) {
    torqueStartLevel1HX = 20;
    torqueStartMaxLevelHX = 5;
  }
}

byte getAssistPercentage(byte level) {
  if (level == 0) return 0;
  return (100 * level) / numAssistLevels;
}

volatile unsigned long lastPulseTime = 0;
volatile unsigned long pulseInterval = 0;  

volatile unsigned long lowStartTime = 0;
volatile bool lowStateStarted = false;

void countPulse() {
  bool state = digitalRead(pasPin);
  unsigned long now = micros();

  if (state == LOW) {
    // FALLING
    lowStartTime = now;
    lowStateStarted = true;
  } else {
    // RISING
    if (lowStateStarted) {
      unsigned long lowDuration = now - lowStartTime;
      lowStateStarted = false;

      //Serial.println(lowDuration);

      // low state is shorter on forward direction
      if (lowDuration < 80000) {
        pulseDetected = true;
      }
    }
  }
}

byte voltageToPWM(int voltageInt) {
  float voltage = voltageInt / 100.0;
  float supplyVoltage = supplyVoltageInt / 100.0;

  if (voltage <= 0) return 0;
  if (voltage >= supplyVoltage) return 255;
  return (byte)((voltage / supplyVoltage) * 255);
}

void printSerialHelp() {
  Serial.println();
  Serial.println("=== Serial control ===");
  Serial.println("u - assist up / setting +");
  Serial.println("d - assist down / setting -");
  Serial.println("s - next setting");
  Serial.println("l - enter/exit settings");
  Serial.println("w - walk assist ON");
  Serial.println("x - walk assist OFF");
  Serial.println("p - PAS pulse");
  Serial.println("? - help");
  Serial.println();
}

void toggleSettingsMode() {
  if (!inSettingsMode) {
    inSettingsMode = true;
    currentSettingIndex = 0;
    lcd.clear();
    showCurrentSetting();
    Serial.println("Settings mode: ON");
  } else {
    inSettingsMode = false;
    saveSettingsToEEPROM();
    lcd.clear();
    Serial.println("Settings mode: OFF");
    showOnDisplay(true);
  }
}

void handleSerialCommands() {
  while (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == '\n' || cmd == '\r') continue;

    switch (cmd) {
      case 'u':
        if (inSettingsMode) {
          changeSetting(true);
          showCurrentSetting();
        } else if (currentAssistLevel < numAssistLevels) {
          currentAssistLevel++;
          targetAssist = getAssistPercentage(currentAssistLevel);
          showOnDisplay(true);
        }
        break;

      case 'd':
        if (inSettingsMode) {
          changeSetting(false);
          showCurrentSetting();
        } else if (currentAssistLevel > 0) {
          currentAssistLevel--;
          targetAssist = getAssistPercentage(currentAssistLevel);
          showOnDisplay(true);
        }
        break;

      case 's':
        if (inSettingsMode) {
          currentSettingIndex = (currentSettingIndex + 1) % totalSettings;
          lcd.clear();
          showCurrentSetting();
        }
        break;

      case 'l':
        toggleSettingsMode();
        break;

      case 'w':
        serialWalkPressed = true;
        Serial.println("Walk assist: ON");
        break;

      case 'x':
        serialWalkPressed = false;
        walkingPressStart = 0;
        walkingAssistActive = false;
        Serial.println("Walk assist: OFF");
        break;

      case 'p':
        pulseDetected = true;
        Serial.println("PAS pulse");
        break;

      case '?':
        printSerialHelp();
        break;
    }
  }
}

void setup() {
  if (lcd.begin()) lcd.print("LCD init...");
  lcd.setCursor(false);
  delay(500);

  lcd.clear();
  lcd.print("EEPROM vars init");
  //saveSettingsToEEPROM();
  loadSettingsFromEEPROM();
  delay(500);

  lcd.clear();
  lcd.print("Pins init...");
  pinMode(pasPin, INPUT_PULLUP);
  pinMode(assistUpPin, INPUT_PULLUP);
  pinMode(assistDownPin, INPUT_PULLUP);
  pinMode(setButtonPin, INPUT_PULLUP);
  pinMode(walkAssistPin, INPUT_PULLUP);
  pinMode(pwmPin, OUTPUT);

  delay(500);

  lcd.clear();
  lcd.print("HX init...");
  if (!hx.begin(3000)) {
    Serial.println("HX init failed");
    lcd.clear();
    lcd.print("HX init error");
    delay(1000);
  }

  attachInterrupt(digitalPinToInterrupt(pasPin), countPulse, CHANGE);


  lastMillis = millis();
  lcd.clear();
}

void loop() {
  handleSerialCommands();
  handleSettings();

  if (!inSettingsMode) {
    samplePAS();
    updateAssistLevel();
    walkingAssist();
    calculateRPM();
    updateHXSensor();
    calculateAssist();
    updatePWM();
    if (lastInSettingsMode){
      showOnDisplay(true);
    }else{
      showOnDisplay(false);
    }
  }else{
    assistPower = 0;
    hybridBasePower = 0;
    hybridCorrection = 0;
    updatePWM();
  }

  lastInSettingsMode = inSettingsMode;
}

void updateHXSensor() {
  bool shouldZero = hxZeroingEnabled && (rpm == 0);

  hx.setZeroing(shouldZero);
  hx.update();

  hxLevel = hx.getLevel();
  hxTorque = hx.getTorque();
  hxError = hx.hasError();
  hxErrorText = hx.getErrorText();
}

void updatePWM() {
  byte pwmIdleVal = voltageToPWM(pwmIdleVoltInt);       
  byte pwmMinVal  = voltageToPWM(pwmMinVoltInt);     
  byte pwmMaxVal  = voltageToPWM(pwmMaxVoltInt);    

  byte pwmValue = pwmIdleVal;

  if (assistPower > 0) {
    pwmValue = map(assistPower, 0, 100, pwmMinVal, pwmMaxVal);
  }

  analogWrite(pwmPin, pwmValue);
}

void samplePAS() {
  unsigned long now = millis();

  // Check if it's time to sample
  if (now - lastSampleTime >= sampleInterval) {
    lastSampleTime = now;

    // Store 1 if pulse was detected since last sample, else 0
    pulseBuffer[bufferIndex] = pulseDetected ? 1 : 0;
    pulseDetected = false; // Reset pulse flag

    // Advance the circular buffer index
    bufferIndex = (bufferIndex + 1) % bufferSize;
  }
}

void walkingAssist() {
  if (currentAssistLevel == 0 && rpm == 0 &&
    (digitalRead(walkAssistPin) == LOW || serialWalkPressed)) {
    if (walkingPressStart == 0) {
      walkingPressStart = millis();
    } else if (!walkingAssistActive && millis() - walkingPressStart >= walkingDelay) {
      walkingAssistActive = true;
      walkingAssistStart = millis(); // start rampy walking assist
    }
  } else {
    walkingPressStart = 0;
    walkingAssistActive = false;
  }

  if (walkingAssistActive) {
    unsigned long elapsed = millis() - walkingAssistStart;
    if (elapsed >= rampTime) {
      assistPower = walkingAssistPower;
    } else {
      assistPower = (walkingAssistPower * elapsed) / rampTime;
    }
  }
}

void updateAssistLevel() {
  static unsigned long lastButtonPress = 0;
  const unsigned long debounceDelay = 200;

  if (millis() - lastButtonPress > debounceDelay) {
    if (digitalRead(assistUpPin) == LOW && currentAssistLevel < numAssistLevels) {
      currentAssistLevel++;
      targetAssist = getAssistPercentage(currentAssistLevel);
      lastButtonPress = millis();
    }
    if (digitalRead(assistDownPin) == LOW && currentAssistLevel > 0) {
      currentAssistLevel--;
      targetAssist = getAssistPercentage(currentAssistLevel);
      lastButtonPress = millis();
    }
  }
}

int getTorqueStartThreshold() {
  if (currentAssistLevel == 0) return 101;

  byte level = currentAssistLevel;

  if (level < 1) level = 1;
  if (level > numAssistLevels) level = numAssistLevels;

  float k = 0.0f;

  if (numAssistLevels > 1) {
    k = (float)(level - 1) / (float)(numAssistLevels - 1);
  }

  // poziom 1 -> torqueStartLevel1HX
  // poziom max -> torqueStartMaxLevelHX
  int startTh = (int)(
    torqueStartLevel1HX -
    ((float)(torqueStartLevel1HX - torqueStartMaxLevelHX) * k) +
    0.5f
  );

  if (startTh < torqueStartMaxLevelHX) startTh = torqueStartMaxLevelHX;
  if (startTh > torqueStartLevel1HX) startTh = torqueStartLevel1HX;

  return startTh;
}

int getTorqueStopThreshold() {
  int stopTh = getTorqueStartThreshold() - torqueStopHysteresis;

  if (stopTh < 0) stopTh = 0;
  return stopTh;
}

void calculateAssist() {
  switch (assistMode) {
    case ASSIST_TORQUE:
      calculateAssistTorque();
      break;

    case ASSIST_HYBRID:
      calculateAssistHybrid();
      break;

    case ASSIST_STANDARD:
    default:
      calculateAssistStandard();
      break;
  }
}

int moveTowardIntAsym(int current, int target, int upStep, int downStep) {
  if (current < target) {
    current += upStep;
    if (current > target) current = target;
  } 
  else if (current > target) {
    current -= downStep;
    if (current < target) current = target;
  }

  return current;
}

int getHybridTargetCorrection() {
  if (hxError) {
    return 0;
  }

  if (hxLevel <= hybridLightHX) {
    return -hybridCorrectionMax;
  }

  if (hxLevel >= hybridHeavyHX) {
    return hybridCorrectionMax;
  }

  if (hxLevel < hybridNeutralHX) {
    return map(
      hxLevel,
      hybridLightHX,
      hybridNeutralHX,
      -hybridCorrectionMax,
      0
    );
  }

  return map(
    hxLevel,
    hybridNeutralHX,
    hybridHeavyHX,
    0,
    hybridCorrectionMax
  );
}

void updateHybridBasePower(int baseAssist) {
  if (hybridBasePower < baseAssist) {
    int step = (baseAssist * updateInterval) / rampTime;

    // zabezpieczenie, żeby przy długim rampTime step nie wyszedł 0
    if (step < 1) step = 1;

    hybridBasePower += step;

    if (hybridBasePower > baseAssist) {
      hybridBasePower = baseAssist;
    }
  } 
  else if (hybridBasePower > baseAssist) {
    // tak jak w STANDARD — po zmniejszeniu poziomu nie czekamy na rampę w dół
    hybridBasePower = baseAssist;
  }
}
void calculateAssistHybrid() {
  unsigned long now = millis();

  if (walkingAssistActive) return;
  if (now - lastUpdate < updateInterval) return;

  lastUpdate = now;

  if (currentAssistLevel == 0 || rpm < minRPM) {
    assistPower = 0;
    hybridBasePower = 0;
    hybridCorrection = 0;
    return;
  }

  int baseAssist = targetAssist;

  // 1. Baza działa jak STANDARD, czyli narasta według rampTime
  updateHybridBasePower(baseAssist);

  // 2. Korekta torque działa osobno
  if (hxError) {
    // Błąd HX: torque przestaje działać, zostaje sama baza
    hybridCorrection = 0;
  } else {
    int targetCorrection = getHybridTargetCorrection();

    hybridCorrection = moveTowardIntAsym(
      hybridCorrection,
      targetCorrection,
      hybridCorrectionUpStepPerUpdate,
      hybridCorrectionDownStepPerUpdate
    );
  }

  // 3. Wynik końcowy = baza + korekta
  int finalPower = hybridBasePower + hybridCorrection;

  if (finalPower < 0) finalPower = 0;
  if (finalPower > 100) finalPower = 100;

  assistPower = finalPower;
}

void calculateAssistStandard() {
  unsigned long now = millis();

  if (walkingAssistActive) return; 

  if (now - lastUpdate < updateInterval) return;  

  lastUpdate = now;

  if (rpm >= minRPM) {
    int step = (targetAssist * updateInterval) / rampTime;
    assistPower += step;
    if (assistPower > targetAssist) assistPower = targetAssist;
} else {
    assistPower = 0;
  }
}

void calculateAssistTorque() {
  unsigned long now = millis();

  if (walkingAssistActive) return;
  if (now - lastUpdate < updateInterval) return;

  lastUpdate = now;

  if (currentAssistLevel == 0 || rpm < minRPM || hxError) {
    assistPower = 0;
    return;
  }

  int startTh = getTorqueStartThreshold();
  int stopTh  = getTorqueStopThreshold();

  const int torqueMaxPower = 100;

  // max wzrost: +1 co 100 ms = 10% na sekundę
  if (hxLevel > startTh) {
    if (assistPower < torqueMaxPower) {
      assistPower += 1;
      if (assistPower > torqueMaxPower) assistPower = torqueMaxPower;
    }
  }
  // spadek szybszy przy slabym nacisku
  else if (hxLevel < stopTh) {
    if (assistPower >= 2) assistPower -= 2;
    else assistPower = 0;
  }

  if (assistPower > torqueMaxPower) assistPower = torqueMaxPower;
}

void calculateRPM() {
  unsigned int pulseCount = 0;

  // Count how many pulses occurred in the sliding window
  for (byte i = 0; i < bufferSize; i++) {
    pulseCount += pulseBuffer[i];
  }

  // Calculate window duration in ms
  unsigned long windowDuration = bufferSize * sampleInterval;

  if (pulseCount > 0) {
    // Convert to pulses per minute, then divide by pulses/revolution to get RPM
    unsigned long pulsesPerMinute = (pulseCount * 60000UL) / windowDuration;
    rpm = pulsesPerMinute / pulsesPerRevolution;
  } else {
    rpm = 0;
  }
}

void showOnDisplay(bool refreshNow) {
  renderScreen(
    lcd,
    currentScreen,
    refreshNow,
    rpm,
    targetAssist,
    assistPower,
    walkingAssistActive,
    hxLevel,
    hxTorque,
    hxError,
    hxErrorText,
    millis(),
    displayRefreshInterval,
    displayState
  );
}

void handleSettings() {
  static bool setButtonHeld = false;
  static bool setButtonPreviouslyPressed = false;

  bool setPressed = digitalRead(setButtonPin) == LOW;

  // Sprawdzenie długiego przytrzymania
  if (setPressed && !setButtonPreviouslyPressed) {
    lastSetButtonPress = millis();
  }

  if (setPressed && (millis() - lastSetButtonPress > longPressTime)) {
    if (!inSettingsMode) {
      // Wejście do ustawień
      inSettingsMode = true;
      currentSettingIndex = 0;
      lcd.clear();
      showCurrentSetting();
    } else {
      // Wyjście z ustawień
      inSettingsMode = false;
      saveSettingsToEEPROM();
      lcd.clear();
    }
    // Zapobiegamy ponownemu wejściu/wyjściu do momentu puszczenia przycisku
    while (digitalRead(setButtonPin) == LOW) delay(10);
  }

  setButtonPreviouslyPressed = setPressed;

  if (inSettingsMode) {
    handleSettingsMenu();
  }
}

void handleSettingsMenu() {
  static unsigned long lastButton = 0;
  const unsigned long debounce = 200;

  if (millis() - lastButton < debounce) return;

  if (digitalRead(assistUpPin) == LOW) {
    changeSetting(true);
    lastButton = millis();
    showCurrentSetting();
  } 
  else if (digitalRead(assistDownPin) == LOW) {
    changeSetting(false);
    lastButton = millis();
    showCurrentSetting();
  } 
  else if (digitalRead(setButtonPin) == LOW) {
    currentSettingIndex = (currentSettingIndex + 1) % totalSettings;
    lcd.clear();
    lastButton = millis();
    showCurrentSetting();
  }
}

void changeSetting(bool increase) {
  switch (currentSettingIndex) {
    case 0:
      if (increase) {
        if (currentScreen + 1 < totalScreens) currentScreen++;
      } else {
        if (currentScreen > 0) currentScreen--;
      }
      break;

    case 1:
      assistMode++;
      if (assistMode > ASSIST_HYBRID) assistMode = ASSIST_STANDARD;
      break;

    case 2:
      hxZeroingEnabled = !hxZeroingEnabled;
      break;

    case 3:
      if (increase) pulsesPerRevolution++;
      else if (pulsesPerRevolution > 1) pulsesPerRevolution--;
      break;

    case 4:
      if (increase && walkingAssistPower < 100) walkingAssistPower++;
      else if (!increase && walkingAssistPower > 1) walkingAssistPower--;
      break;

    case 5:
      if (increase) walkingDelay += 500;
      else if (walkingDelay >= 1000) walkingDelay -= 500;
      break;

    case 6:
      if (increase) minRPM++;
      else if (minRPM > 0) minRPM--;
      break;

    case 7:
      if (increase) rampTime += 200;
      else if (rampTime >= 400) rampTime -= 200;
      break;

    case 8:
      if (increase && numAssistLevels < 20) {
        numAssistLevels++;
      } else if (!increase && numAssistLevels > 1) {
        numAssistLevels--;
      }

      if (currentAssistLevel > numAssistLevels) {
        currentAssistLevel = numAssistLevels;
      }

      targetAssist = getAssistPercentage(currentAssistLevel);
      break;

    case 9:
      if (increase && sampleInterval < 500) sampleInterval += 10;
      else if (!increase && sampleInterval >= 20) sampleInterval -= 10;
      break;

    case 10:
      if (increase && pwmMinVoltInt < 490) pwmMinVoltInt += 10;
      else if (!increase && pwmMinVoltInt > 10) pwmMinVoltInt -= 10;
      break;

    case 11:
      if (increase && pwmMaxVoltInt < 500) pwmMaxVoltInt += 10;
      else if (!increase && pwmMaxVoltInt > pwmMinVoltInt + 10) pwmMaxVoltInt -= 10;
      break;

    case 12:
      if (increase && pwmIdleVoltInt < 500) pwmIdleVoltInt += 10;
      else if (!increase && pwmIdleVoltInt > pwmMinVoltInt + 10) pwmIdleVoltInt -= 10;
      break;

    case 13:
      if (increase && supplyVoltageInt < 500) supplyVoltageInt += 10;
      else if (!increase && supplyVoltageInt > 300) supplyVoltageInt -= 10;
      break;

    case 14:
      // Hybrid Light HX
      if (increase && hybridLightHX < hybridNeutralHX - 1) {
        hybridLightHX++;
      } else if (!increase && hybridLightHX > 0) {
        hybridLightHX--;
      }
      break;

    case 15:
      // Hybrid Neutral HX
      if (increase && hybridNeutralHX < hybridHeavyHX - 1) {
        hybridNeutralHX++;
      } else if (!increase && hybridNeutralHX > hybridLightHX + 1) {
        hybridNeutralHX--;
      }
      break;

    case 16:
      // Hybrid Heavy HX
      if (increase && hybridHeavyHX < 100) {
        hybridHeavyHX++;
      } else if (!increase && hybridHeavyHX > hybridNeutralHX + 1) {
        hybridHeavyHX--;
      }
      break;

    case 17:
      // Torque Level 1 HX
      if (increase && torqueStartLevel1HX < 100) {
        torqueStartLevel1HX++;
      } else if (!increase && torqueStartLevel1HX > torqueStartMaxLevelHX + 1) {
        torqueStartLevel1HX--;
      }
      break;

    case 18:
      // Torque Max Level HX
      if (increase && torqueStartMaxLevelHX < torqueStartLevel1HX - 1) {
        torqueStartMaxLevelHX++;
      } else if (!increase && torqueStartMaxLevelHX > 1) {
        torqueStartMaxLevelHX--;
      }
      break;

    case 19:
      // Torque hysteresis
      if (increase && torqueStopHysteresis < 20) {
        torqueStopHysteresis++;
      } else if (!increase && torqueStopHysteresis > 1) {
        torqueStopHysteresis--;
      }
      break;
  }
}

void showCurrentSetting() {
  static String lastLine0 = "";
  static String lastLine1 = "";
  static unsigned long lastCallTime = 0;

  String line0 = "";
  String line1 = "";

  switch (currentSettingIndex) {
    case 0:
      line0 = "Screen:";
      line1 = String(currentScreen) + "/" + String(totalScreens - 1);
      break;

    case 1:
      line0 = "Assist Mode:";

      if (assistMode == ASSIST_STANDARD) {
        line1 = "STANDARD";
      } else if (assistMode == ASSIST_TORQUE) {
        line1 = "TORQUE";
      } else if (assistMode == ASSIST_HYBRID) {
        line1 = "HYBRID";
      }

      break;

    case 2:
      line0 = "HX Zeroing:";
      line1 = hxZeroingEnabled ? "ON" : "OFF";
      break;

    case 3:
      line0 = "NumOf Magnetics:";
      line1 = String(pulsesPerRevolution);
      break;

    case 4:
      line0 = "Walk Asst [%]:";
      line1 = String(walkingAssistPower);
      break;

    case 5:
      line0 = "WalkDelay [ms]:";
      line1 = String(walkingDelay);
      break;

    case 6:
      line0 = "Min RPM:";
      line1 = String(minRPM);
      break;

    case 7:
      line0 = "RampTime [ms]:";
      line1 = String(rampTime);
      break;

    case 8:
      line0 = "Asst Levels:";
      line1 = String(numAssistLevels);
      break;

    case 9:
      line0 = "RPMIntval [ms]:";
      line1 = String(sampleInterval);
      break;

    case 10:
      line0 = "PWM Min Volt:";
      line1 = String(pwmMinVoltInt / 100.0, 1) + " V";
      break;

    case 11:
      line0 = "PWM Max Volt:";
      line1 = String(pwmMaxVoltInt / 100.0, 1) + " V";
      break;

    case 12:
      line0 = "PWM Idle Volt:";
      line1 = String(pwmIdleVoltInt / 100.0, 1) + " V";
      break;

    case 13:
      line0 = "Supply Volt:";
      line1 = String(supplyVoltageInt / 100.0, 2) + " V";
      break;

    case 14:
      line0 = "Hyb Light HX:";
      line1 = String(hybridLightHX);
      break;

    case 15:
      line0 = "Hyb Neutral HX:";
      line1 = String(hybridNeutralHX);
      break;

    case 16:
      line0 = "Hyb Heavy HX:";
      line1 = String(hybridHeavyHX);
      break;

    case 17:
      line0 = "Torq L1 HX:";
      line1 = String(torqueStartLevel1HX);
      break;

    case 18:
      line0 = "Torq Max HX:";
      line1 = String(torqueStartMaxLevelHX);
      break;

    case 19:
      line0 = "Torq Hyst:";
      line1 = String(torqueStopHysteresis);
      break;
  }

  while (line0.length() < 16) line0 += ' ';
  while (line1.length() < 16) line1 += ' ';
  if (line0.length() > 16) line0 = line0.substring(0, 16);
  if (line1.length() > 16) line1 = line1.substring(0, 16);

  unsigned long now = millis();
  bool forceRedraw = (now - lastCallTime > 350);
  lastCallTime = now;

  if (forceRedraw || line0 != lastLine0) {
    lcd.setCursor(0, 0);
    lcd.print(line0);
    lastLine0 = line0;
  }

  if (forceRedraw || line1 != lastLine1) {
    lcd.setCursor(0, 1);
    lcd.print(line1);
    lastLine1 = line1;
  }
}