#pragma once

#include "Config.h"
#include <Preferences.h>

// Estatísticas persistentes de treinamento
class StatisticsManager {
public:
    void begin();
    void load();
    void save();
    void clearAll();

    void onSessionStart();
    void onSessionEnd(uint32_t sessionMs);
    void onCharacterSent(bool correct);
    void onStreakUpdate(uint16_t streak);
    void onWpmAchieved(uint8_t wpm);

    uint32_t totalSessions;
    uint32_t totalTrainingMs;
    uint32_t charsSent;
    uint32_t charsCorrect;
    uint32_t charsIncorrect;
    uint16_t bestStreak;
    uint8_t  bestWpm;
    uint32_t lastUsedUnix;   // millis/1000 desde boot como aproximação

    float trainingHours() const;
    float accuracyPercent() const;

private:
    Preferences _prefs;
    static constexpr const char* NS = "cwstats";
    uint32_t _sessionStartMs;
};
