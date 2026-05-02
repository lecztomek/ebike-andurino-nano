#pragma once
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

struct ScreenDisplayState {
  int lastRPM = 10;
  int lastTargetAssist = 0;
  int lastAssistPower = 0;

  int lastHxLevel = -1;
  long lastHxTorque = 0;
  bool lastHxError = false;
  char lastHxStatus[17] = {0};

  unsigned long lastDisplayUpdateTime = 0;
  byte lastScreen = 255;
};

extern const uint8_t UI_BIG_TRON_GLYPHS[8][8];
extern const uint8_t UI_BIG_TRON_DIGITS[11][4];

void buildScreenLines(
  byte currentScreen,
  unsigned int rpm,
  unsigned int targetAssist,
  unsigned int assistPower,
  bool walkingAssistActive,
  int hxLevel,
  long hxTorque,
  bool hxError,
  const char* hxStatusText,
  char line1[17],
  char line2[17]
);

template <typename TLcd>
static void uiLoadBigTronFont(TLcd& lcd) {
  for (byte i = 0; i < 8; i++) {
    uint8_t glyph[8];
    for (byte r = 0; r < 8; r++) {
      glyph[r] = UI_BIG_TRON_GLYPHS[i][r];
    }
    lcd.setGlyph(i, glyph);
  }
}

template <typename TLcd>
static void uiPrintBigDigit2x2(TLcd& lcd, byte startCol, const uint8_t tiles[4]) {
  lcd.setCursor(startCol + 0, 0);
  lcd.print((uint8_t)tiles[0]); // UL

  lcd.setCursor(startCol + 0, 1);
  lcd.print((uint8_t)tiles[1]); // LL

  lcd.setCursor(startCol + 1, 0);
  lcd.print((uint8_t)tiles[2]); // UR

  lcd.setCursor(startCol + 1, 1);
  lcd.print((uint8_t)tiles[3]); // LR
}

template <typename TLcd>
static void uiPrintBig2DigitValue(TLcd& lcd, byte startCol, unsigned int value) {
  if (value > 99) value = 99;

  byte tens = value / 10;
  byte ones = value % 10;

  const uint8_t* leftDigit  = (value >= 10) ? UI_BIG_TRON_DIGITS[tens] : UI_BIG_TRON_DIGITS[10]; // blank
  const uint8_t* rightDigit = UI_BIG_TRON_DIGITS[ones];

  uiPrintBigDigit2x2(lcd, startCol + 0, leftDigit);
  uiPrintBigDigit2x2(lcd, startCol + 3, rightDigit);
}

template <typename TLcd>
static void uiRenderBigValueScreen(TLcd& lcd, const char* label, unsigned int value) {
  uiLoadBigTronFont(lcd);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(label);

  char buf[6];
  snprintf(buf, sizeof(buf), "%u", value);

  byte len = 0;
  while (buf[len] != '\0' && len < 5) len++;

  if (len == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    len = 1;
  }

  byte labelWidth = 0;
  while (label[labelWidth] != '\0' && labelWidth < 16) labelWidth++;

  // 1 duza cyfra = 2 kolumny, miedzy cyframi 1 kolumna przerwy
  byte totalWidth = len * 2 + (len - 1);

  // Start od prawej, ale nie wchodz na napis
  byte minStartCol = labelWidth;
  if (minStartCol < 15) minStartCol += 1;

  byte startCol = 16 - totalWidth;
  if (startCol < minStartCol) startCol = minStartCol;

  // Gdy sie nie miesci, fallback do zwyklego tekstu
  if (startCol + totalWidth > 16) {
    lcd.setCursor(minStartCol < 16 ? minStartCol : 0, 0);
    lcd.print(buf);
    return;
  }

  for (byte i = 0; i < len; i++) {
    byte digit = (byte)(buf[i] - '0');
    if (digit > 9) digit = 10;

    uiPrintBigDigit2x2(
      lcd,
      startCol + i * 3,
      UI_BIG_TRON_DIGITS[digit]
    );
  }
}

template <typename TLcd>
static void uiRenderScreen2TA(TLcd& lcd, unsigned int targetAssist) {
  uiRenderBigValueScreen(lcd, "ASSIST", targetAssist);
}

template <typename TLcd>
static void uiRenderScreen3RPM(TLcd& lcd, unsigned int rpm) {
  uiRenderBigValueScreen(lcd, "RPM", rpm);
}

template <typename TLcd>
static void uiRenderScreen4HXLevelBig(
  TLcd& lcd,
  int hxLevel,
  bool hxError,
  const char* hxStatusText
) {
  uiLoadBigTronFont(lcd);
  lcd.clear();

  if (hxLevel < 0) hxLevel = 0;
  if (hxLevel > 999) hxLevel = 999;

  const char* safeStatus = (hxStatusText && hxStatusText[0]) ? hxStatusText : (hxError ? "ERR" : "OK");

  // lewa strona - status
  lcd.setCursor(0, 0);
  lcd.print("LEVEL");

  char shortStatus[8];
  snprintf(shortStatus, sizeof(shortStatus), "%.7s", safeStatus);
  lcd.setCursor(0, 1);
  lcd.print(shortStatus);

  // prawa strona - duzy level
  char buf[5];
  snprintf(buf, sizeof(buf), "%d", hxLevel);

  byte len = 0;
  while (buf[len] != '\0' && len < 4) len++;

  if (len == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    len = 1;
  }

  byte totalWidth = len * 2 + (len - 1);

  // zostawiamy lewa czesc na status
  byte minStartCol = 8;
  byte startCol = 16 - totalWidth;
  if (startCol < minStartCol) startCol = minStartCol;

  // fallback do zwyklego tekstu
  if (startCol + totalWidth > 16) {
    lcd.setCursor(minStartCol, 0);
    lcd.print(buf);
    return;
  }

  for (byte i = 0; i < len; i++) {
    byte digit = (byte)(buf[i] - '0');
    if (digit > 9) digit = 10;

    uiPrintBigDigit2x2(
      lcd,
      startCol + i * 3,
      UI_BIG_TRON_DIGITS[digit]
    );
  }
}

template <typename TLcd>
static void uiRenderScreen5TargetVsAssistPower(
  TLcd& lcd,
  unsigned int targetAssist,
  unsigned int assistPower
) {
  uiLoadBigTronFont(lcd);
  lcd.clear();

  // lewa wielka liczba: targetAssist
  uiPrintBig2DigitValue(lcd, 0, targetAssist);

  // srodek male napisy
  lcd.setCursor(5, 0);
  lcd.print("<-tgtA");

  lcd.setCursor(5, 1);
  lcd.print("asst->");

  // prawa wielka liczba: assistPower
  uiPrintBig2DigitValue(lcd, 11, assistPower);
}

template <typename TLcd>
static void uiRenderScreen6HxLevelVsAssistPower(
  TLcd& lcd,
  int hxLevel,
  unsigned int assistPower
) {
  uiLoadBigTronFont(lcd);
  lcd.clear();

  if (hxLevel < 0) hxLevel = 0;
  if (hxLevel > 99) hxLevel = 99;

  // lewa wielka liczba: hxLevel
  uiPrintBig2DigitValue(lcd, 0, (unsigned int)hxLevel);

  // srodek male napisy
  lcd.setCursor(5, 0);
  lcd.print("<-lvl ");

  lcd.setCursor(5, 1);
  lcd.print("asst->");

  // prawa wielka liczba: assistPower
  uiPrintBig2DigitValue(lcd, 11, assistPower);
}

template <typename TLcd>
void renderScreen(
  TLcd& lcd,
  byte currentScreen,
  bool refreshNow,
  unsigned int rpm,
  unsigned int targetAssist,
  unsigned int assistPower,
  bool walkingAssistActive,
  int hxLevel,
  long hxTorque,
  bool hxError,
  const char* hxStatusText,
  unsigned long currentTime,
  unsigned long displayRefreshInterval,
  ScreenDisplayState& displayState
) {
  const char* safeHxStatusText = hxStatusText ? hxStatusText : "";

  bool screenChanged = (currentScreen != displayState.lastScreen);

  bool statusChanged =
    (strncmp(
      displayState.lastHxStatus,
      safeHxStatusText,
      sizeof(displayState.lastHxStatus) - 1
    ) != 0);

  bool valueChanged =
    rpm != displayState.lastRPM ||
    targetAssist != displayState.lastTargetAssist ||
    assistPower != displayState.lastAssistPower ||
    hxLevel != displayState.lastHxLevel ||
    hxTorque != displayState.lastHxTorque ||
    hxError != displayState.lastHxError ||
    statusChanged;

  bool intervalElapsed =
    (currentTime - displayState.lastDisplayUpdateTime >= displayRefreshInterval);

  if (!(screenChanged || ((refreshNow || valueChanged) && intervalElapsed))) {
    return;
  }

  // 0 = small main
  // 1 = small HX
  // 2 = big ASSIST
  // 3 = big RPM
  // 4 = big HX LEVEL + status
  // 5 = big TARGET vs ASSIST POWER
  // 6 = big HX LEVEL vs ASSIST POWER
  if (currentScreen == 2) {
    uiRenderScreen2TA(lcd, targetAssist);
  } else if (currentScreen == 3) {
    uiRenderScreen3RPM(lcd, rpm);
  } else if (currentScreen == 4) {
    uiRenderScreen4HXLevelBig(lcd, hxLevel, hxError, safeHxStatusText);
  } else if (currentScreen == 5) {
    uiRenderScreen5TargetVsAssistPower(lcd, targetAssist, assistPower);
  } else if (currentScreen == 6) {
    uiRenderScreen6HxLevelVsAssistPower(lcd, hxLevel, assistPower);
  } else {
    char line1[17];
    char line2[17];

    buildScreenLines(
      currentScreen,
      rpm,
      targetAssist,
      assistPower,
      walkingAssistActive,
      hxLevel,
      hxTorque,
      hxError,
      safeHxStatusText,
      line1,
      line2
    );

    if (screenChanged) {
      lcd.clear();
    }

    lcd.setCursor(0, 0);
    lcd.print(line1);

    lcd.setCursor(0, 1);
    lcd.print(line2);
  }

  displayState.lastRPM = rpm;
  displayState.lastTargetAssist = targetAssist;
  displayState.lastAssistPower = assistPower;

  displayState.lastHxLevel = hxLevel;
  displayState.lastHxTorque = hxTorque;
  displayState.lastHxError = hxError;

  strncpy(
    displayState.lastHxStatus,
    safeHxStatusText,
    sizeof(displayState.lastHxStatus) - 1
  );
  displayState.lastHxStatus[sizeof(displayState.lastHxStatus) - 1] = '\0';

  displayState.lastDisplayUpdateTime = currentTime;
  displayState.lastScreen = currentScreen;
}