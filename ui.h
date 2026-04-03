#pragma once
#include <Arduino.h>
#include <stdio.h>

struct ScreenDisplayState {
  int lastRPM = 10;
  int lastTargetAssist = 0;
  int lastAssistPower = 0;
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
static void uiRenderScreen2TA(TLcd& lcd, unsigned int targetAssist) {
  uiLoadBigTronFont(lcd);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("ASSIST:");

  char buf[6];
  snprintf(buf, sizeof(buf), "%u", targetAssist);

  byte len = 0;
  while (buf[len] != '\0' && len < 5) len++;

  if (len == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    len = 1;
  }

  // 1 duza cyfra = 2 kolumny, miedzy cyframi 1 kolumna przerwy
  // 4 cyfry jeszcze wchodza obok "TA:"
  if (len > 4) {
    lcd.setCursor(4, 0);
    lcd.print(buf);
    return;
  }

  byte totalWidth = len * 2 + (len - 1);
  byte startCol = 16 - totalWidth;
  if (startCol < 4) startCol = 4;

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
void renderScreen(
  TLcd& lcd,
  byte currentScreen,
  bool refreshNow,
  unsigned int rpm,
  unsigned int targetAssist,
  unsigned int assistPower,
  bool walkingAssistActive,
  unsigned long currentTime,
  unsigned long displayRefreshInterval,
  ScreenDisplayState& displayState
) {
  bool screenChanged = (currentScreen != displayState.lastScreen);
  bool valueChanged =
    rpm != displayState.lastRPM ||
    targetAssist != displayState.lastTargetAssist ||
    assistPower != displayState.lastAssistPower;

  bool intervalElapsed =
    (currentTime - displayState.lastDisplayUpdateTime >= displayRefreshInterval);

  if (!(screenChanged || ((refreshNow || valueChanged) && intervalElapsed))) {
    return;
  }

  // drugi ekran, czyli currentScreen == 1
  if (currentScreen == 1) {
    uiRenderScreen2TA(lcd, targetAssist);
  } else {
    char line1[17];
    char line2[17];

    buildScreenLines(
      currentScreen,
      rpm,
      targetAssist,
      assistPower,
      walkingAssistActive,
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
  displayState.lastDisplayUpdateTime = currentTime;
  displayState.lastScreen = currentScreen;
}