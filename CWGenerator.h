#pragma once

#include "Config.h"
#include <M5Unified.h>

// Sidetone e reprodução Morse — tom contínuo ON/OFF (sem engasgos)
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
    enum class Phase : uint8_t { Idle, Tone, ElementGap, LetterGap };

    uint16_t _freq;
    uint8_t  _volume;
    bool     _paddleActive;
    bool     _playActive;
    Phase    _phase;
    uint32_t _phaseEndMs;
    uint8_t  _wpm;

    char     _pattern[16];
    uint8_t  _patIdx;

    char     _text[24];
    uint8_t  _textLen;
    uint8_t  _textIdx;
    bool     _multiChar;

    void applyVolume();
    void speakerOn();
    void speakerOff();
    void loadCharPattern(char c);
    void startElement(uint32_t nowMs);
    void finishAll();
};
