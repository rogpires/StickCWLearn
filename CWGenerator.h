#pragma once

#include "Config.h"
#include <M5Unified.h>

// Geração de sidetone e reprodução de Morse no alto-falante interno (ES8311 / I2S)
class CWGenerator {
public:
    void begin();
    void configure(uint16_t frequencyHz, uint8_t volumePercent);
    void update(uint32_t nowMs);

    // Sidetone ao manipular o paddle
    void toneOn();
    void toneOff();
    bool isPaddleActive() const;

    // Reproduz caractere em Morse pelo speaker (não bloqueante)
    void playCharacter(char c, uint8_t wpm);
    void stopPlayback();
    bool isPlaying() const;

    void playFeedback(bool success);

private:
    enum class PlayPhase : uint8_t { Idle, Tone, Gap };

    uint16_t _freq;
    uint8_t  _volume;
    bool     _paddleActive;

    // Reprodução automática de padrão Morse
    char     _playPattern[16];
    uint8_t  _playPos;
    uint8_t  _playWpm;
    PlayPhase _playPhase;
    uint32_t _phaseEndMs;
    bool     _playActive;

    void applyVolume();
    void beginElement(uint32_t nowMs);
    void finishPlayback();
};
