#pragma once

#include "Config.h"
#include "SettingsManager.h"
#include <functional>

// Leitura de Paddle/Straight Key com keyer iâmbico integrado
class CWInput {
public:
    using EventCallback = std::function<void(const KeyEvent&)>;

    void begin();
    void configure(const SettingsManager& settings);
    void setCallback(EventCallback cb);
    void update(uint32_t nowMs);

    bool isToneActive() const;
    MorseElement lastCompletedElement() const;
    uint32_t lastElementDurationMs() const;

    // Histórico para modo Professor (últimos elementos)
    struct ElementRecord {
        MorseElement type;
        uint32_t durationMs;
        uint32_t gapAfterMs;
    };

    static constexpr uint8_t HISTORY_SIZE = 16;
    uint8_t historyCount() const;
    const ElementRecord* historyAt(uint8_t index) const;

private:
    EventCallback _callback;
    PaddleMode _mode;
    bool _invert;
    uint8_t _wpm;

    // Estado GPIO
    bool _ditRaw;
    bool _dahRaw;
    bool _ditStable;
    bool _dahStable;
    uint32_t _ditLastChange;
    uint32_t _dahLastChange;

    // Straight key
    bool _straightDown;
    uint32_t _straightDownMs;

    // Keyer iâmbico
    enum class KeyerState : uint8_t {
        IDLE,
        SENDING_ELEMENT,
        INTER_ELEMENT_GAP
    };

    KeyerState _keyerState;
    MorseElement _sendingElement;
    MorseElement _nextIambicElement;
    uint32_t _stateStartMs;
    bool _toneActive;
    bool _ditLatch;
    bool _dahLatch;

    MorseElement _lastCompleted;
    uint32_t _lastDuration;

    ElementRecord _history[HISTORY_SIZE];
    uint8_t _historyHead;
    uint8_t _historyCount;
    uint32_t _lastElementEndMs;

    bool readGpio(uint8_t pin) const;
    void debounce(uint32_t nowMs);
    void updateStraightKey(uint32_t nowMs);
    void updateIambic(uint32_t nowMs);
    void startElement(MorseElement el, uint32_t nowMs);
    void finishElement(uint32_t nowMs);
    void emitEvent(const KeyEvent& ev);
    void pushHistory(MorseElement type, uint32_t dur, uint32_t gap);
};
