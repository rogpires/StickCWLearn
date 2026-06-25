#pragma once

#include "Config.h"
#include <M5Unified.h>

// Sidetone CW via M5.Speaker.tone() (ES8311 / I2S — não é buzzer piezo).
// Tom contínuo + envelope por volume do canal — evita stop/start entre elementos.
class CWGenerator {
public:
    void begin();
    void configure(uint16_t frequencyHz, uint8_t volumePercent);
    void update(uint32_t nowMs);

    void toneOn();
    void toneOff();
    bool isPaddleActive() const;

    void playCharacter(char c, uint8_t wpm);
    void playText(const char* text, uint8_t wpm);
    void stopPlayback();
    bool isPlaying() const;

private:
    enum class Phase : uint8_t {
        Idle, Attack, Sustain, Release, SilencePad, ElementGap, LetterGap,
        PaddleRelease
    };

    uint16_t _freq;
    uint8_t  _volume;
    uint8_t  _volTarget;
    uint8_t  _volCurrent;

    bool     _paddleActive;
    bool     _playActive;
    bool     _toneRunning;
    Phase    _phase;
    uint32_t _phaseEndMs;
    uint8_t  _wpm;
    uint8_t  _envStep;

    char     _pattern[16];
    uint8_t  _patIdx;

    char     _text[24];
    uint8_t  _textLen;
    uint8_t  _textIdx;
    bool     _multiChar;

    void applyVolumeTarget();
    void setOutputVolume(uint8_t vol);
    void startOscillator();
    void stopOscillator();
    void loadCharPattern(char c);
    void startElement(uint32_t nowMs);
    void finishAll();
    uint32_t elementDurationMs(char sym) const;
    uint32_t sustainMs(uint32_t totalMs) const;
};
