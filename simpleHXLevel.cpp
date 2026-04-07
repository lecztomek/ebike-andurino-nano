#include "simpleHXLevel.h"

SimpleHXLevel::SimpleHXLevel(uint8_t dtPin, uint8_t sckPin, uint8_t pulses)
  : _dtPin(dtPin), _sckPin(sckPin), _pulses(pulses) {
}

long SimpleHXLevel::absL(long v) {
  return (v < 0) ? -v : v;
}

bool SimpleHXLevel::begin(uint32_t startupCalMs) {
  pinMode(_sckPin, OUTPUT);
  digitalWrite(_sckPin, LOW);
  pinMode(_dtPin, INPUT);

  long sum = 0;
  int n = 0;
  uint32_t t0 = millis();

  while ((uint32_t)(millis() - t0) < startupCalMs) {
    long r;
    bool timeout = false;

    if (readStable(r, timeout) && !isSaturated(r)) {
      sum += r;
      n++;
    }

    delay(20);
  }

  _zeroF = (n > 0) ? (float)(sum / n) : 0.0f;
  _torqueF = 0.0f;
  _level = 0;
  _latchedError = false;
  _error = ERR_NONE;
  _okCount = 0;
  _winInit = false;
  _softStartUntil = millis() + _softStartMs;
  _lastGoodReadMs = millis();

  return (n > 0);
}

void SimpleHXLevel::setZeroing(bool on) {
  _zeroing = on;
}

uint32_t SimpleHXLevel::read24(bool &timeoutFlag) {
  timeoutFlag = false;

  uint32_t t0 = millis();
  while (digitalRead(_dtPin) == HIGH) {
    if ((uint32_t)(millis() - t0) > _readTimeoutMs) {
      timeoutFlag = true;
      return 0;
    }
  }

  uint32_t data = 0;

  noInterrupts();

  for (uint8_t i = 0; i < 24; i++) {
    digitalWrite(_sckPin, HIGH);
    delayMicroseconds(1);
    data = (data << 1) | (digitalRead(_dtPin) ? 1 : 0);
    digitalWrite(_sckPin, LOW);
    delayMicroseconds(1);
  }

  for (uint8_t i = 0; i < _pulses; i++) {
    digitalWrite(_sckPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(_sckPin, LOW);
    delayMicroseconds(1);
  }

  interrupts();

  return data;
}

long SimpleHXLevel::toSigned24(uint32_t d) {
  if (d & 0x800000UL) d |= 0xFF000000UL;
  return (long)d;
}

bool SimpleHXLevel::isSaturated(long v) {
  return (absL(v - _satPos) <= _satMargin) || (absL(v - _satNeg) <= _satMargin);
}

bool SimpleHXLevel::readStable(long &outRaw, bool &timeoutFlag) {
  uint32_t d = read24(timeoutFlag);
  if (timeoutFlag) return false;

  outRaw = toSigned24(d);
  return true;
}

int SimpleHXLevel::level100(long dAbs) {
  if (dAbs <= _dMin) return 0;
  if (dAbs >= _dMax) return 100;

  long num = (dAbs - _dMin) * 100L;
  long den = (_dMax - _dMin);
  int lvl = (int)(num / den);

  if (lvl < 0) lvl = 0;
  if (lvl > 100) lvl = 100;
  return lvl;
}

void SimpleHXLevel::enterError(ErrorCode err) {
  _latchedError = true;
  _error = err;
  _okCount = 0;
  _level = 0;
}

void SimpleHXLevel::tryRecover() {
  if (_okCount >= _okRequired) {
    _latchedError = false;
    _error = ERR_NONE;
    _okCount = 0;
    _softStartUntil = millis() + _softStartMs;
    _winInit = false;
  }
}

bool SimpleHXLevel::update() {
  uint32_t now = millis();
  if ((uint32_t)(now - _lastUpdateMs) < _periodMs) return false;
  _lastUpdateMs = now;

  long raw = 0;
  bool timeout = false;
  bool okRead = readStable(raw, timeout);

  if (okRead) {
    _raw = raw;
  }

  bool saturated = okRead && isSaturated(raw);
  bool goodSample = okRead && !saturated;

  // natychmiastowy błąd przy saturacji
  if (saturated) {
    enterError(ERR_SATURATION);
  }

  // poprawna próbka zeruje licznik braku danych
  if (goodSample) {
    _lastGoodReadMs = now;
  } else {
    // timeout dopiero po 1 sekundzie bez poprawnego odczytu
    if ((uint32_t)(now - _lastGoodReadMs) >= _signalTimeoutMs) {
      enterError(ERR_TIMEOUT);
    }
  }

  if (_latchedError) {
    if (goodSample) {
      long torque = raw - (long)_zeroF;
      _torqueF = _torqueF + _torqueAlpha * ((float)torque - _torqueF);

      long tAbs = absL((long)_torqueF);
      int lvl = level100(tAbs);

      if (lvl <= _recoverLevelMax) _okCount++;
      else _okCount = 0;

      tryRecover();
    } else {
      _okCount = 0;
    }

    _level = 0;
    return true;
  }

  // jeśli chwilowo nie ma próbki, ale nie minęła jeszcze 1 sekunda,
  // to po prostu zostaw poprzedni stan i nie zgłaszaj błędu
  if (!goodSample) {
    return true;
  }

  long torque = raw - (long)_zeroF;
  _torqueF = _torqueF + _torqueAlpha * ((float)torque - _torqueF);
  long tAbs = absL((long)_torqueF);

  if (tAbs > _torqueMax) {
    enterError(ERR_TORQUE_LIMIT);
    return true;
  }

  if (!_winInit) {
    _winMin = (long)_torqueF;
    _winMax = (long)_torqueF;
    _winInit = true;
    _winStart = now;
  }

  if ((long)_torqueF < _winMin) _winMin = (long)_torqueF;
  if ((long)_torqueF > _winMax) _winMax = (long)_torqueF;

  if ((uint32_t)(now - _winStart) >= 1000) {
    long pp = _winMax - _winMin;
    if (pp > _jitterMax) {
      enterError(ERR_JITTER);
      return true;
    }
    _winStart = now;
    _winInit = false;
  }

  if (_zeroing) {
    _zeroF = _zeroF + _zeroAlphaRun * ((float)raw - _zeroF);
  }

  _level = (millis() < _softStartUntil) ? 0 : level100(tAbs);
  return true;
}

int SimpleHXLevel::getLevel() const {
  return _latchedError ? 0 : _level;
}

long SimpleHXLevel::getTorque() const {
  return _latchedError ? 0 : (long)_torqueF;
}

bool SimpleHXLevel::hasError() const {
  return _latchedError;
}

SimpleHXLevel::ErrorCode SimpleHXLevel::getError() const {
  return _error;
}

const char* SimpleHXLevel::getErrorText() const {
  switch (_error) {
    case ERR_NONE:         return "OK";
    case ERR_TIMEOUT:      return "TIMEOUT";
    case ERR_SATURATION:   return "SATURATION";
    case ERR_TORQUE_LIMIT: return "TORQUE_LIMIT";
    case ERR_JITTER:       return "JITTER";
    default:               return "UNKNOWN";
  }
}