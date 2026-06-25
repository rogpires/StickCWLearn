#include "MorseDecoder.h"
#include <cstring>
#include <cctype>

// Tabela Morse completa: letras, números e símbolos
static const char* MORSE_TABLE[] = {
    ".-",   "-...",  "-.-.",  "-..",   ".",     "..-.",  "--.",   "....",
    "..",   ".---",  "-.-",   ".-..",  "--",    "-.",    "---",   ".--.",
    "--.-", ".-..",  "-..--", "..--",  "-.--",  "--..",
    "-----",".----","..---","...--","....-",".....",
    "-....","--...","---..","----.",
    ".-.-.-","--..--","..--..","-..-.","-...-",".-.-.","-....-",".--.-.",
    "---...","-.-.-.","-.--."
};

static const char MORSE_CHARS[] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    '0','1','2','3','4','5','6','7','8','9',
    '.',',','?','/','=','+','-','@',':',';'
};

static constexpr size_t MORSE_TABLE_SIZE = sizeof(MORSE_CHARS);

void MorseDecoder::reset() {
    _wpm = WPM_DEFAULT;
    _seqLen = 0;
    _sequence[0] = '\0';
    _textLen = 0;
    _text[0] = '\0';
    _lastChar = '\0';
    _lastElement = MorseElement::NONE;
    _lastActivityMs = 0;
    _newChar = false;
    _wordEnded = false;
}

void MorseDecoder::setWpm(uint8_t wpm) {
    _wpm = wpm;
}

void MorseDecoder::onElement(MorseElement element) {
    _lastElement = element;
    _lastActivityMs = millis();
    appendToSequence(element == MorseElement::DIT ? '.' : '-');
}

void MorseDecoder::update(uint32_t nowMs) {
    if (_seqLen == 0 || _lastActivityMs == 0) return;

    uint32_t elapsed = nowMs - _lastActivityMs;
    uint32_t letterGap = letterGapMs(_wpm);
    uint32_t wordGap = wordGapMs(_wpm);

    if (elapsed >= wordGap) {
        finalizeCharacter();
        _wordEnded = true;
        _lastActivityMs = 0;
    } else if (elapsed >= letterGap) {
        finalizeCharacter();
    }
}

const char* MorseDecoder::currentSequence() const {
    return _sequence;
}

const char* MorseDecoder::decodedText() const {
    return _text;
}

char MorseDecoder::lastDecodedChar() const {
    return _lastChar;
}

MorseElement MorseDecoder::lastElement() const {
    return _lastElement;
}

bool MorseDecoder::hasNewChar() const {
    return _newChar;
}

void MorseDecoder::clearNewCharFlag() {
    _newChar = false;
}

bool MorseDecoder::wordJustEnded() const {
    return _wordEnded;
}

void MorseDecoder::clearWordFlag() {
    _wordEnded = false;
}

char MorseDecoder::morseToChar(const char* pattern) {
    if (!pattern || pattern[0] == '\0') return '?';
    for (size_t i = 0; i < MORSE_TABLE_SIZE; ++i) {
        if (strcmp(pattern, MORSE_TABLE[i]) == 0) {
            return MORSE_CHARS[i];
        }
    }
    return '?';
}

bool MorseDecoder::charToMorse(char c, char* buf, size_t bufSize) {
    if (!buf || bufSize == 0) return false;
    char upper = toupper(c);
    for (size_t i = 0; i < MORSE_TABLE_SIZE; ++i) {
        if (MORSE_CHARS[i] == upper || MORSE_CHARS[i] == c) {
            strncpy(buf, MORSE_TABLE[i], bufSize - 1);
            buf[bufSize - 1] = '\0';
            return true;
        }
    }
    buf[0] = '\0';
    return false;
}

void MorseDecoder::appendToSequence(char symbol) {
    if (_seqLen < sizeof(_sequence) - 1) {
        _sequence[_seqLen++] = symbol;
        _sequence[_seqLen] = '\0';
    }
}

void MorseDecoder::finalizeCharacter() {
    if (_seqLen == 0) return;

    char c = morseToChar(_sequence);
    _lastChar = c;
    appendChar(c);
    _newChar = true;

    _seqLen = 0;
    _sequence[0] = '\0';
}

void MorseDecoder::appendChar(char c) {
    if (_textLen < SESSION_MAX_CHARS) {
        _text[_textLen++] = c;
        _text[_textLen] = '\0';
    } else {
        // Desloca buffer (rolagem)
        memmove(_text, _text + 1, SESSION_MAX_CHARS - 1);
        _text[SESSION_MAX_CHARS - 1] = c;
        _text[SESSION_MAX_CHARS] = '\0';
    }
}
