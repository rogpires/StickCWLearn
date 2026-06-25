#include "CWGenerator.h"
#include "MorseDecoder.h"

void CWGenerator::begin() {
    _freq = SIDETONE_FREQ_DEFAULT;
    _volume = 90;
    _paddleActive = false;
    _playActive = false;
    _playPhase = PlayPhase::Idle;
    _playPos = 0;
    applyVolume();
}

void CWGenerator::configure(uint16_t frequencyHz, uint8_t volumePercent) {
    _freq = frequencyHz;
    _volume = volumePercent;
    applyVolume();
}

void CWGenerator::applyVolume() {
    // M5StickS3: faixa ampla para speaker interno audível (128–255)
    uint8_t vol = map(_volume, 0, 100, 128, 255);
    M5.Speaker.setVolume(vol);
}

void CWGenerator::toneOn() {
    if (_paddleActive) return;
    stopPlayback();
    _paddleActive = true;
    applyVolume();
    M5.Speaker.tone(_freq, 60000);
}

void CWGenerator::toneOff() {
    if (!_paddleActive) return;
    _paddleActive = false;
    M5.Speaker.stop();
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
    _playPhase = PlayPhase::Idle;
    _playPos = 0;
    if (!_paddleActive) {
        M5.Speaker.stop();
    }
}

void CWGenerator::playCharacter(char c, uint8_t wpm) {
    if (_paddleActive) return;
    if (!MorseDecoder::charToMorse(c, _playPattern, sizeof(_playPattern))) return;
    if (_playPattern[0] == '\0') return;

    stopPlayback();
    _playWpm = wpm;
    _playPos = 0;
    _playActive = true;
    beginElement(millis());
}

void CWGenerator::beginElement(uint32_t nowMs) {
    char sym = _playPattern[_playPos];
    if (sym == '\0') {
        finishPlayback();
        return;
    }

    uint32_t dur = (sym == '.') ? ditDurationMs(_playWpm) : dahDurationMs(_playWpm);
    applyVolume();
    M5.Speaker.tone(_freq, dur + 200);
    _playPhase = PlayPhase::Tone;
    _phaseEndMs = nowMs + dur;
}

void CWGenerator::update(uint32_t nowMs) {
    if (!_playActive || _paddleActive) return;
    if (nowMs < _phaseEndMs) return;

    if (_playPhase == PlayPhase::Tone) {
        M5.Speaker.stop();
        _playPhase = PlayPhase::Gap;
        _phaseEndMs = nowMs + elementGapMs(_playWpm);
        return;
    }

    if (_playPhase == PlayPhase::Gap) {
        _playPos++;
        if (_playPattern[_playPos] == '\0') {
            finishPlayback();
            return;
        }
        beginElement(nowMs);
    }
}

void CWGenerator::finishPlayback() {
    _playActive = false;
    _playPhase = PlayPhase::Idle;
    _playPos = 0;
    if (!_paddleActive) {
        M5.Speaker.stop();
    }
}

void CWGenerator::playFeedback(bool success) {
    stopPlayback();
    applyVolume();
    if (success) {
        M5.Speaker.tone(_freq + 200, 80);
    } else {
        M5.Speaker.tone(_freq - 150, 200);
    }
}
