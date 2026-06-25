#pragma once

#include "Config.h"
#include "MorseDecoder.h"
#include "CWInput.h"
#include "SettingsManager.h"
#include "StatisticsManager.h"
#include <Preferences.h>

class TrainerManager {
public:
    void begin(StatisticsManager* stats);
    void setMode(AppMode mode);
    AppMode mode() const;

    void onKeyEvent(const KeyEvent& ev);
    void update(uint32_t nowMs);
    void onPlaybackFinished();

    const char* statusText() const;
    const char* feedbackText() const;
    char targetChar() const;
    const char* targetWord() const;
    const char* targetCallsign() const;
    const char* currentTarget() const;
    uint16_t score() const;
    uint16_t streak() const;
    uint8_t targetProgress() const;

    void clearFreeText();
    void saveFreeSession();
    void loadLastFreeSession();
    const char* freeText() const;

    static const char* modeName(AppMode m);

    MorseDecoder& decoder();
    const MorseDecoder& decoder() const;
    void setWpm(uint8_t wpm);

    void replayTarget();
    bool pendingPlayback() const;
    bool playbackIsText() const;
    char playbackChar() const;
    const char* playbackText() const;
    void clearPlaybackPending();

private:
    enum class PlaybackType : uint8_t { None, Char, Text };

    AppMode _mode;
    char _status[48];
    char _feedback[80];
    char _targetWord[16];
    char _targetCallsign[12];
    char _targetMorse[16];
    char _targetChar;
    uint16_t _score;
    uint16_t _streak;
    uint16_t _attempts;
    uint8_t _wordCharIndex;
    uint8_t _wordIndex;

    char _freeText[SESSION_MAX_CHARS + 1];
    uint16_t _freeLen;

    MorseDecoder _decoder;
    Preferences _prefs;
    StatisticsManager* _stats;

    PlaybackType _playPending;
    char _playChar;
    char _playText[20];

    static const char* WORD_BANK[];
    static constexpr uint8_t WORD_COUNT = 10;

    void requestPlayChar(char c);
    void requestPlayText(const char* text);
    void schedulePlaybackForCurrentTarget();

    void pickRandomLetter();
    void pickRandomNumber();
    void pickRandomWord();
    void generateCallsign();
    void evaluateCharacter(char decoded);
    void setFeedback(const char* msg);
    void setStatus(const char* msg);
};
