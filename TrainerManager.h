#pragma once

#include "Config.h"
#include "MorseDecoder.h"
#include "CWInput.h"
#include "SettingsManager.h"
#include "StatisticsManager.h"
#include <Preferences.h>

// Gerencia todos os modos de treino, avaliação e modo Professor
class TrainerManager {
public:
    void begin(StatisticsManager* stats);
    void setMode(AppMode mode);
    AppMode mode() const;

    void onKeyEvent(const KeyEvent& ev);
    void update(uint32_t nowMs);

    // Estado da UI
    const char* statusText() const;
    const char* feedbackText() const;
    char targetChar() const;
    const char* targetWord() const;
    const char* targetCallsign() const;
    uint16_t score() const;
    uint16_t streak() const;
    uint16_t totalAttempts() const;
    bool awaitingInput() const;

    // Modo livre
    void clearFreeText();
    void saveFreeSession();
    void loadLastFreeSession();
    const char* freeText() const;

    // Professor
    void analyzeManipulation(const CWInput& input, uint8_t wpm);

    // Utilitários
    static const char* modeName(AppMode m);

    void resetTrainingRound();

    MorseDecoder& decoder();
    const MorseDecoder& decoder() const;
    void setWpm(uint8_t wpm);

    // Reprodução automática no speaker (letras e números)
    void replayTarget();
    bool pendingMorsePlayback() const;
    char playbackChar() const;
    void clearPlaybackPending();

private:
    AppMode _mode;
    char _status[48];
    char _feedback[80];
    char _targetWord[16];
    char _targetCallsign[12];
    char _targetMorse[16];
    char _inputMorse[16];
    uint8_t _inputMorseLen;
    char _targetChar;
    uint16_t _score;
    uint16_t _streak;
    uint16_t _attempts;
    uint8_t _wordIndex;
    uint8_t _wordCharIndex;
    bool _awaiting;
    bool _evaluated;
    uint32_t _roundStartMs;

    char _freeText[SESSION_MAX_CHARS + 1];
    uint16_t _freeLen;

    MorseDecoder _decoder;
    Preferences _prefs;
    StatisticsManager* _stats;

    bool _playTargetPending;
    char _playTargetChar;

    void requestTargetPlayback(char c);

    static const char* WORD_BANK[];
    static constexpr uint8_t WORD_COUNT = 10;

    void pickRandomLetter();
    void pickRandomNumber();
    void pickRandomWord();
    void generateCallsign();
    void evaluateCharacter(char decoded);
    void setFeedback(const char* msg);
    void setStatus(const char* msg);
    char randomCharFromPool(const char* pool);
};
