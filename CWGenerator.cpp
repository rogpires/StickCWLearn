#include "CWGenerator.h"
#include "MorseDecoder.h"
#include <cstring>

void CWGenerator::begin() {
    _freq = SIDETONE_FREQ_DEFAULT;
    _volume = 90;
    _paddleActive = false;
    _playActive = false;
    _phase = Phase::Idle;
    applyVolume();
}

void CWGenerator::configure(uint16_t frequencyHz, uint8_t volumePercent) {
    _freq = frequencyHz;
    _volume = volumePercent;
    applyVolume();
}

void CWGenerator::applyVolume() {
    uint8_t vol = map(_volume, 0, 100, 160, 255);
    M5.Speaker.setVolume(vol);
}

void CWGenerator::speakerOn() {
    applyVolume();
    M5.Speaker.tone(_freq, 60000);
}

void CWGenerator::speakerOff() {
    M5.Speaker.stop();
}

void CWGenerator::toneOn() {
    if (_paddleActive) return;
    stopPlayback();
    _paddleActive = true;
    speakerOn();
}

void CWGenerator::toneOff() {
    if (!_paddleActive) return;
    _paddleActive = false;
    speakerOff();
}

bool CWGenerator::isPaddleActive() const {
    return _paddleActive;
}

bool CWGenerator::isPlaying() const {
    return _playActive || _paddleActive;
}

void CWGenerator::stopPlayback() {
    if (!_playActive) return;
    _playActive = false;
    _phase = Phase::Idle;
    _patIdx = _textIdx = _textLen = 0;
    _multiChar = false;
    if (!_paddleActive) speakerOff();
}

void CWGenerator::loadCharPattern(char c) {
    _pattern[0] = '\0';
    MorseDecoder::charToMorse(c, _pattern, sizeof(_pattern));
    _patIdx = 0;
}

void CWGenerator::playCharacter(char c, uint8_t wpm) {
    if (_paddleActive || c == '\0') return;
    stopPlayback();
    _wpm = wpm;
    _multiChar = false;
    _playActive = true;
    loadCharPattern(c);
    startElement(millis());
}

void CWGenerator::playText(const char* text, uint8_t wpm) {
    if (_paddleActive || !text || text[0] == '\0') return;
    stopPlayback();
    strncpy(_text, text, sizeof(_text) - 1);
    _text[sizeof(_text) - 1] = '\0';
    _textLen = strlen(_text);
    _textIdx = 0;
    _wpm = wpm;
    _multiChar = (_textLen > 1);
    _playActive = true;
    loadCharPattern(_text[0]);
    startElement(millis());
}

void CWGenerator::startElement(uint32_t nowMs) {
    if (_patIdx >= sizeof(_pattern) || _pattern[_patIdx] == '\0') {
        if (_multiChar && (_textIdx + 1) < _textLen) {
            _phase = Phase::LetterGap;
            _phaseEndMs = nowMs + letterGapMs(_wpm);
            speakerOff();
        } else {
            finishAll();
        }
        return;
    }

    char sym = _pattern[_patIdx];
    uint32_t dur = (sym == '.') ? ditDurationMs(_wpm) : dahDurationMs(_wpm);
    speakerOn();
    _phase = Phase::Tone;
    _phaseEndMs = nowMs + dur;
}

void CWGenerator::update(uint32_t nowMs) {
    if (!_playActive || _paddleActive) return;
    if (nowMs < _phaseEndMs) return;

    switch (_phase) {
        case Phase::Tone:
            speakerOff();
            _phase = Phase::ElementGap;
            _phaseEndMs = nowMs + elementGapMs(_wpm);
            break;

        case Phase::ElementGap:
            _patIdx++;
            startElement(nowMs);
            break;

        case Phase::LetterGap:
            _textIdx++;
            loadCharPattern(_text[_textIdx]);
            startElement(nowMs);
            break;

        default:
            finishAll();
            break;
    }
}

void CWGenerator::finishAll() {
    _playActive = false;
    _phase = Phase::Idle;
    if (!_paddleActive) speakerOff();
}
