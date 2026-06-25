#include "CWGenerator.h"
#include "MorseDecoder.h"
#include <cstring>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

void CWGenerator::begin() {
    _freq = SIDETONE_FREQ_DEFAULT;
    _volume = 90;
    _paddleActive = false;
    _playActive = false;
    _toneRunning = false;
    _phase = Phase::Idle;
    _volCurrent = 0;
    rebuildSineTable();
    applyVolumeTarget();

    auto spkCfg = M5.Speaker.config();
    spkCfg.magnification = 2;
    M5.Speaker.config(spkCfg);
}

void CWGenerator::rebuildSineTable() {
    for (size_t i = 0; i < CW_SINE_SAMPLES; ++i) {
        float t = (2.0f * PI * (float)i) / (float)CW_SINE_SAMPLES;
        _sine[i] = (int16_t)(sinf(t) * 14000.0f);
    }
    _sampleRate = (uint32_t)_freq * CW_SINE_SAMPLES;
    if (_sampleRate < 4000) _sampleRate = 4000;
}

void CWGenerator::configure(uint16_t frequencyHz, uint8_t volumePercent) {
    bool freqChanged = (frequencyHz != _freq);
    _freq = frequencyHz;
    _volume = volumePercent;
    applyVolumeTarget();
    if (freqChanged) {
        rebuildSineTable();
        if (_toneRunning) {
            stopOscillator();
            startOscillator();
        }
    }
}

void CWGenerator::applyVolumeTarget() {
    _volTarget = map(_volume, 0, 100, 120, 220);
}

void CWGenerator::setOutputVolume(uint8_t vol) {
    _volCurrent = vol;
    M5.Speaker.setVolume(vol);
}

void CWGenerator::startOscillator() {
    if (_toneRunning) return;
    setOutputVolume(0);
    M5.Speaker.playRaw(_sine, CW_SINE_SAMPLES, _sampleRate, false,
                       UINT32_MAX, 16);
    _toneRunning = true;
}

void CWGenerator::stopOscillator() {
    if (!_toneRunning) return;
    M5.Speaker.stop();
    _toneRunning = false;
    setOutputVolume(0);
}

uint32_t CWGenerator::elementDurationMs(char sym) const {
    return (sym == '.') ? ditDurationMs(_wpm) : dahDurationMs(_wpm);
}

uint32_t CWGenerator::sustainMs(uint32_t totalMs) const {
    uint32_t overhead = CW_ATTACK_MS + CW_RELEASE_MS;
    if (totalMs > overhead + 8) return totalMs - overhead;
    return totalMs / 2;
}

void CWGenerator::toneOn() {
    if (_paddleActive) return;
    stopPlayback();
    _paddleActive = true;
    _envStep = 0;
    startOscillator();
    _phase = Phase::Attack;
    _phaseEndMs = millis() + (CW_ATTACK_MS / CW_ATTACK_STEPS);
    setOutputVolume(0);
}

void CWGenerator::toneOff() {
    if (!_paddleActive) return;
    _paddleActive = false;
    if (!_toneRunning) {
        _phase = Phase::Idle;
        return;
    }
    _envStep = 0;
    _phase = Phase::PaddleRelease;
    _phaseEndMs = millis() + (CW_RELEASE_MS / CW_RELEASE_STEPS);
}

bool CWGenerator::isPaddleActive() const {
    return _paddleActive;
}

bool CWGenerator::isPlaying() const {
    return _playActive || _paddleActive || _toneRunning
        || _phase == Phase::Attack || _phase == Phase::Sustain
        || _phase == Phase::Release || _phase == Phase::PaddleRelease;
}

void CWGenerator::stopPlayback() {
    _playActive = false;
    _phase = Phase::Idle;
    _patIdx = _textIdx = _textLen = 0;
    _multiChar = false;
    if (!_paddleActive) {
        stopOscillator();
    }
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
        } else {
            finishAll();
        }
        return;
    }

    startOscillator();
    _envStep = 0;
    _phase = Phase::Attack;
    _phaseEndMs = nowMs + (CW_ATTACK_MS / CW_ATTACK_STEPS);
    setOutputVolume(0);
}

void CWGenerator::update(uint32_t nowMs) {
    if (_phase == Phase::Idle) return;

    if (nowMs < _phaseEndMs) {
        return;
    }

    switch (_phase) {
        case Phase::Attack: {
            _envStep++;
            if (_envStep >= CW_ATTACK_STEPS) {
                setOutputVolume(_volTarget);
                _phase = Phase::Sustain;
                if (_paddleActive && !_playActive) {
                    _phaseEndMs = nowMs + 300000UL;
                } else {
                    char sym = _pattern[_patIdx];
                    _phaseEndMs = nowMs + sustainMs(elementDurationMs(sym));
                }
            } else {
                uint8_t vol = (uint8_t)((uint16_t)_volTarget * (_envStep + 1) / CW_ATTACK_STEPS);
                setOutputVolume(vol);
                _phaseEndMs = nowMs + (CW_ATTACK_MS / CW_ATTACK_STEPS);
            }
            break;
        }

        case Phase::Sustain:
            if (_paddleActive && !_playActive) {
                break;
            }
            _envStep = 0;
            _phase = Phase::Release;
            _phaseEndMs = nowMs + (CW_RELEASE_MS / CW_RELEASE_STEPS);
            break;

        case Phase::Release:
        case Phase::PaddleRelease: {
            _envStep++;
            if (_envStep >= CW_RELEASE_STEPS) {
                stopOscillator();
                if (_phase == Phase::PaddleRelease) {
                    _phase = Phase::Idle;
                    break;
                }
                _phase = Phase::SilencePad;
                _phaseEndMs = nowMs + CW_SILENCE_PAD_MS;
            } else {
                uint8_t vol = (uint8_t)((uint16_t)_volTarget * (CW_RELEASE_STEPS - _envStep) / CW_RELEASE_STEPS);
                setOutputVolume(vol);
                _phaseEndMs = nowMs + (CW_RELEASE_MS / CW_RELEASE_STEPS);
            }
            break;
        }

        case Phase::SilencePad:
            if (!_playActive) {
                _phase = Phase::Idle;
                break;
            }
            _phase = Phase::ElementGap;
            {
                uint32_t gap = elementGapMs(_wpm);
                if (gap > CW_SILENCE_PAD_MS) gap -= CW_SILENCE_PAD_MS;
                else gap = 1;
                _phaseEndMs = nowMs + gap;
            }
            break;

        case Phase::ElementGap:
            if (!_playActive) break;
            _patIdx++;
            startElement(nowMs);
            break;

        case Phase::LetterGap:
            if (!_playActive) break;
            _textIdx++;
            loadCharPattern(_text[_textIdx]);
            startElement(nowMs);
            break;

        default:
            break;
    }
}

void CWGenerator::finishAll() {
    _playActive = false;
    if (_toneRunning) {
        _envStep = 0;
        _phase = Phase::Release;
        _phaseEndMs = millis() + (CW_RELEASE_MS / CW_RELEASE_STEPS);
    } else {
        _phase = Phase::Idle;
    }
}
