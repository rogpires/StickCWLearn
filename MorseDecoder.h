#pragma once

#include "Config.h"
#include <Arduino.h>

// Decodificador Morse com temporização baseada em WPM (padrão PARIS)
class MorseDecoder {
public:
    void reset();
    void setWpm(uint8_t wpm);

    // Chamado quando um elemento (dit/dah) é completado pelo keyer
    void onElement(MorseElement element);

    // Chamado no loop para detectar fim de caractere/palavra por timeout
    void update(uint32_t nowMs);

    const char* currentSequence() const;
    const char* decodedText() const;
    char lastDecodedChar() const;
    MorseElement lastElement() const;
    bool hasNewChar() const;
    void clearNewCharFlag();
    bool wordJustEnded() const;
    void clearWordFlag();

    // Decodifica padrão Morse para caractere ('?' se desconhecido)
    static char morseToChar(const char* pattern);
    static bool charToMorse(char c, char* buf, size_t bufSize);

private:
    uint8_t  _wpm;
    char     _sequence[16];
    uint8_t  _seqLen;
    char     _text[SESSION_MAX_CHARS + 1];
    uint16_t _textLen;
    char     _lastChar;
    MorseElement _lastElement;
    uint32_t _lastActivityMs;
    bool     _newChar;
    bool     _wordEnded;

    void appendToSequence(char symbol);
    void finalizeCharacter();
    void appendChar(char c);
};
