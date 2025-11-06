#include <Arduino.h>

class Btn {
public:
  // click_ms: クリックとみなす最大押下時間
  // long_ms : 長押しとみなす閾値
  // debounce_ms: デバウンス時間
  Btn(uint16_t click_ms = 5, uint16_t long_ms = 300, uint16_t debounce_ms = 2)
    : _pin(255), _activeLow(true),
      _debounceMs(debounce_ms), _clickMs(click_ms), _longMs(long_ms),
      _rawLast(false), _stable(false), _lastStable(false),
      _lastRawChange(0), _lastStableChange(0),
      _pressedAt(0), _releasedAt(0),
      _fTrigger(false), _fRelease(false), _fClick(false), _fLong(false), _longFired(false) {}

  // pullup = true のとき INPUT_PULLUP を使い、"押下=LOW" とみなす
  void begin(uint8_t pin, bool pullup = true) {
    _pin = pin;
    _activeLow = pullup;
    pinMode(_pin, pullup ? INPUT_PULLUP : INPUT);
    bool raw = readRaw();
    _rawLast = raw;
    _stable = raw;         // 起動時は生読みをそのまま安定状態に
    _lastStable = _stable;
    _lastRawChange = millis();
    _lastStableChange = millis();
    if (isOn()) _pressedAt = _lastStableChange;
    else        _releasedAt = _lastStableChange;
  }

  // 毎ループ呼ぶ
  void update() {
    // 直前フレームのイベントフラグをリセット
    _fTrigger = _fRelease = _fClick = _fLong = false;

    // --- デバウンス（生読みの変化がdebounce ms 続いたら安定遷移とみなす）---
    bool raw = readRaw();
    uint32_t now = millis();
    if (raw != _rawLast) {
      _rawLast = raw;
      _lastRawChange = now;
    }

    // rawが変わってから一定時間変化が継続
    if ((now - _lastRawChange) >= _debounceMs && _stable != raw) {
      _lastStable = _stable;
      _stable = raw;
      _lastStableChange = now;

      // 立ち上がり/立ち下がりイベント
      if (isOn()) {
        _pressedAt = now;
        _longFired = false;       // 新規押下で長押し未発火に戻す
        _fTrigger = true;         // デバウンス後の立ち上がり
      } else {
        _releasedAt = now;
        _fRelease = true;         // デバウンス後の立ち下がり

        // クリック判定：押下〜解放が click_ms 以内、かつ長押し未発火
        uint32_t held = _releasedAt - _pressedAt;
        if (!_longFired && held >= _debounceMs && held <= _clickMs) {
          _fClick = true;         // デバウンスを満たす短押し
        }
      }
    }

    // --- 長押し単発判定（押下継続中に閾値を越えた瞬間に1回だけtrue）---
    if (isOn() && !_longFired) {
      if ((now - _pressedAt) >= _longMs) {
        _fLong = true;
        _longFired = true;        // 二重発火を防止
      }
    }
  }

  // 現在（デバウンス済み）の押下状態
  bool on()  const { return isOn(); }
  bool off() const { return !isOn(); }

  // 立ち上がり（デバウンス後にOFF→ONへ変わったフレームのみtrue）
  bool trigger()   const { return _fTrigger; }

  // 立ち下がり（デバウンス後にON→OFFへ変わったフレームのみtrue）
  bool release()   const { return _fRelease; }

  // 短押し（押下→解放までが click_ms 以内、かつ長押し未発火）
  bool click()     const { return _fClick; }

  // 長押し（押下継続時間が long_ms を超えた瞬間のフレームでtrue、単発）
  bool longpress() const { return _fLong; }

  // おまけ：現在の押下継続時間[ms]
  uint32_t heldMs() const {
    uint32_t now = millis();
    return isOn() ? (now - _pressedAt) : 0;
  }

private:
  uint8_t  _pin;
  bool     _activeLow;

  // 時間パラメータ
  uint16_t _debounceMs;
  uint16_t _clickMs;
  uint16_t _longMs;

  // デバウンス用
  bool     _rawLast;
  bool     _stable;
  bool     _lastStable;
  uint32_t _lastRawChange;
  uint32_t _lastStableChange;

  // 押下/解放タイムスタンプ
  uint32_t _pressedAt;
  uint32_t _releasedAt;

  // エッジ/ジェスチャーフラグ（update()毎に更新）
  bool     _fTrigger;
  bool     _fRelease;
  bool     _fClick;
  bool     _fLong;
  bool     _longFired;

  inline bool readRaw() const {
    int v = digitalRead(_pin);
    // activeLowなら LOW=押下(true) に反転
    return _activeLow ? (v == LOW) : (v == HIGH);
  }

  inline bool isOn() const { return _stable; }
};
