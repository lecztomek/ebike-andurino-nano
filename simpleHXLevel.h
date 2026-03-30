#ifndef SIMPLE_HX_LEVEL_H
#define SIMPLE_HX_LEVEL_H

#include <Arduino.h>

class SimpleHXLevel {
public:
  enum ErrorCode : uint8_t {
    ERR_NONE = 0,
    ERR_TIMEOUT,
    ERR_SATURATION,
    ERR_TORQUE_LIMIT,
    ERR_JITTER
  };

  SimpleHXLevel(uint8_t dtPin, uint8_t sckPin, uint8_t pulses = 1);

  bool begin(uint32_t startupCalMs = 2000);
  bool update();                  // wołaj często w loop()
  void setZeroing(bool on);       // zewnętrzne zerowanie

  int  getLevel() const;          // 0..100
  long getTorque() const;
  bool hasError() const;
  ErrorCode getError() const;
  const char* getErrorText() const;

private:
  uint32_t read24(bool &timeoutFlag);
  long toSigned24(uint32_t d);
  bool isSaturated(long v);
  bool readStable(long &outRaw, bool &timeoutFlag);
  int level100(long dAbs);
  void enterError(ErrorCode err);
  void tryRecover();

  static long absL(long v);

private:
  uint8_t _dtPin;
  uint8_t _sckPin;
  uint8_t _pulses;

  // ustawienia z Twojego programu
  long _dMin = 2500;
  long _dMax = 1000000;

  float _torqueAlpha = 0.25f;
  float _zeroAlphaRun = 0.20f;

  uint32_t _readTimeoutMs = 120;

  const long _satPos =  8388607L;
  const long _satNeg = -8388608L;
  const long _satMargin = 2L;

  long _torqueMax = 700000L;
  long _jitterMax = 120000L;

  uint16_t _okRequired = 10;
  int _recoverLevelMax = 2;
  uint32_t _periodMs = 50;
  uint32_t _softStartMs = 2000;

  // stan
  float _zeroF = 0.0f;
  float _torqueF = 0.0f;
  long _raw = 0;
  int _level = 0;

  bool _zeroing = false;

  bool _latchedError = false;
  ErrorCode _error = ERR_NONE;
  uint16_t _okCount = 0;
  uint32_t _softStartUntil = 0;
  uint32_t _lastUpdateMs = 0;

  // jitter window
  long _winMin = 0;
  long _winMax = 0;
  uint32_t _winStart = 0;
  bool _winInit = false;
};

#endif