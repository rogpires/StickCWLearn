#include "CWInput.h"

void CWInput::begin() {
    pinMode(GPIO_DIT, INPUT_PULLUP);
    pinMode(GPIO_DAH, INPUT_PULLUP);

    _keyerState = KeyerState::IDLE;
    _toneActive = false;
    _straightDown = false;
    _historyHead = _historyCount = 0;
    _lastCompleted = MorseElement::NONE;
    _lastDuration = 0;
    _lastElementEndMs = 0;
    _nextIambicElement = MorseElement::DIT;
}

void CWInput::configure(const SettingsManager& settings) {
    _mode = settings.paddleMode;
    _invert = settings.invertContacts;
    _wpm = settings.wpm;
}

void CWInput::setCallback(EventCallback cb) {
    _callback = cb;
}

bool CWInput::readGpio(uint8_t pin) const {
    bool level = digitalRead(pin) == LOW;  // contato fecha para GND
    return _invert ? !level : level;
}

void CWInput::debounce(uint32_t nowMs) {
    bool ditNow = readGpio(GPIO_DIT);
    bool dahNow = readGpio(GPIO_DAH);

    if (ditNow != _ditRaw) {
        _ditRaw = ditNow;
        _ditLastChange = nowMs;
    }
    if (dahNow != _dahRaw) {
        _dahRaw = dahNow;
        _dahLastChange = nowMs;
    }

    if ((nowMs - _ditLastChange) >= DEBOUNCE_MS) {
        _ditStable = _ditRaw;
    }
    if ((nowMs - _dahLastChange) >= DEBOUNCE_MS) {
        _dahStable = _dahRaw;
    }
}

void CWInput::update(uint32_t nowMs) {
    debounce(nowMs);

    if (_mode == PaddleMode::STRAIGHT_KEY) {
        updateStraightKey(nowMs);
    } else {
        updateIambic(nowMs);
    }
}

bool CWInput::isToneActive() const {
    return _toneActive;
}

MorseElement CWInput::lastCompletedElement() const {
    return _lastCompleted;
}

uint32_t CWInput::lastElementDurationMs() const {
    return _lastDuration;
}

uint8_t CWInput::historyCount() const {
    return _historyCount;
}

const CWInput::ElementRecord* CWInput::historyAt(uint8_t index) const {
    if (index >= _historyCount) return nullptr;
    uint8_t pos = (_historyHead + HISTORY_SIZE - 1 - index) % HISTORY_SIZE;
    return &_history[pos];
}

void CWInput::updateStraightKey(uint32_t nowMs) {
    bool keyDown = _ditStable || _dahStable;

    if (keyDown && !_straightDown) {
        _straightDown = true;
        _straightDownMs = nowMs;
        _toneActive = true;
        emitEvent({KeyEventType::TONE_ON, MorseElement::NONE, 0});
    } else if (!keyDown && _straightDown) {
        _straightDown = false;
        uint32_t dur = nowMs - _straightDownMs;
        _toneActive = false;
        emitEvent({KeyEventType::TONE_OFF, MorseElement::NONE, dur});

        uint32_t ditMs = ditDurationMs(_wpm);
        MorseElement el = (dur >= (ditMs + ditMs / 2)) ? MorseElement::DAH : MorseElement::DIT;
        uint32_t gap = (_lastElementEndMs > 0) ? (nowMs - _lastElementEndMs) : 0;

        _lastCompleted = el;
        _lastDuration = dur;
        pushHistory(el, dur, gap);
        _lastElementEndMs = nowMs;

        emitEvent({KeyEventType::ELEMENT_COMPLETE, el, dur});
    }
}

void CWInput::updateIambic(uint32_t nowMs) {
    bool dit = _ditStable;
    bool dah = _dahStable;

    // Latch de paddles durante envio
    if (_keyerState == KeyerState::IDLE) {
        _ditLatch = dit;
        _dahLatch = dah;
    } else {
        if (dit) _ditLatch = true;
        if (dah) _dahLatch = true;
    }

  switch (_keyerState) {
        case KeyerState::IDLE: {
            MorseElement toSend = MorseElement::NONE;

            if (_ditLatch && _dahLatch) {
                // Squeeze — alternância iâmbica
                if (_mode == PaddleMode::IAMBIC_A) {
                    // Iambic A: mantém sequência
                } else {
                    // Iambic B: sempre começa com DIT
                    _nextIambicElement = MorseElement::DIT;
                }
                toSend = _nextIambicElement;
                _nextIambicElement = (toSend == MorseElement::DIT) ? MorseElement::DAH : MorseElement::DIT;
            } else if (_ditLatch) {
                toSend = MorseElement::DIT;
            } else if (_dahLatch) {
                toSend = MorseElement::DAH;
            }

            if (toSend != MorseElement::NONE) {
                startElement(toSend, nowMs);
            }
            break;
        }

        case KeyerState::SENDING_ELEMENT: {
            uint32_t target = (_sendingElement == MorseElement::DIT)
                ? ditDurationMs(_wpm) : dahDurationMs(_wpm);
            if ((nowMs - _stateStartMs) >= target) {
                finishElement(nowMs);
                _keyerState = KeyerState::INTER_ELEMENT_GAP;
                _stateStartMs = nowMs;
            }
            break;
        }

        case KeyerState::INTER_ELEMENT_GAP: {
            if ((nowMs - _stateStartMs) >= elementGapMs(_wpm)) {
                _keyerState = KeyerState::IDLE;
                // Verifica se paddle ainda pressionado
                if (!_ditLatch && !_dahLatch) {
                    _ditLatch = _ditStable;
                    _dahLatch = _dahStable;
                }
            }
            break;
        }
    }
}

void CWInput::startElement(MorseElement el, uint32_t nowMs) {
    _sendingElement = el;
    _keyerState = KeyerState::SENDING_ELEMENT;
    _stateStartMs = nowMs;
    _toneActive = true;
    emitEvent({KeyEventType::TONE_ON, el, 0});
}

void CWInput::finishElement(uint32_t nowMs) {
    uint32_t dur = nowMs - _stateStartMs;
    _toneActive = false;
    emitEvent({KeyEventType::TONE_OFF, _sendingElement, dur});

    uint32_t gap = (_lastElementEndMs > 0) ? (_stateStartMs - _lastElementEndMs) : 0;
    _lastCompleted = _sendingElement;
    _lastDuration = dur;
    pushHistory(_sendingElement, dur, gap);
    _lastElementEndMs = nowMs;

    emitEvent({KeyEventType::ELEMENT_COMPLETE, _sendingElement, dur});

    // Libera latch do paddle correspondente
    if (_sendingElement == MorseElement::DIT) _ditLatch = _ditStable;
    if (_sendingElement == MorseElement::DAH) _dahLatch = _dahStable;
}

void CWInput::emitEvent(const KeyEvent& ev) {
    if (_callback) _callback(ev);
}

void CWInput::pushHistory(MorseElement type, uint32_t dur, uint32_t gap) {
    _history[_historyHead] = {type, dur, gap};
    _historyHead = (_historyHead + 1) % HISTORY_SIZE;
    if (_historyCount < HISTORY_SIZE) _historyCount++;
}
