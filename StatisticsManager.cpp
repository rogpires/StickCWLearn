#include "StatisticsManager.h"

void StatisticsManager::begin() {
    _prefs.begin(NS, false);
    load();
    _sessionStartMs = 0;
}

void StatisticsManager::load() {
    totalSessions   = _prefs.getUInt("sessions", 0);
    totalTrainingMs = _prefs.getUInt("trainMs", 0);
    charsSent       = _prefs.getUInt("sent", 0);
    charsCorrect    = _prefs.getUInt("correct", 0);
    charsIncorrect  = _prefs.getUInt("wrong", 0);
    bestStreak      = _prefs.getUShort("streak", 0);
    bestWpm         = _prefs.getUChar("bestWpm", 0);
    lastUsedUnix    = _prefs.getUInt("lastUse", 0);
}

void StatisticsManager::save() {
    _prefs.putUInt("sessions", totalSessions);
    _prefs.putUInt("trainMs", totalTrainingMs);
    _prefs.putUInt("sent", charsSent);
    _prefs.putUInt("correct", charsCorrect);
    _prefs.putUInt("wrong", charsIncorrect);
    _prefs.putUShort("streak", bestStreak);
    _prefs.putUChar("bestWpm", bestWpm);
    _prefs.putUInt("lastUse", lastUsedUnix);
}

void StatisticsManager::clearAll() {
    totalSessions = totalTrainingMs = charsSent = 0;
    charsCorrect = charsIncorrect = 0;
    bestStreak = 0;
    bestWpm = 0;
    lastUsedUnix = 0;
    save();
}

void StatisticsManager::onSessionStart() {
    _sessionStartMs = millis();
    totalSessions++;
    lastUsedUnix = millis() / 1000UL;
    save();
}

void StatisticsManager::onSessionEnd(uint32_t sessionMs) {
    totalTrainingMs += sessionMs;
    lastUsedUnix = millis() / 1000UL;
    save();
}

void StatisticsManager::onCharacterSent(bool correct) {
    charsSent++;
    if (correct) charsCorrect++;
    else charsIncorrect++;
    lastUsedUnix = millis() / 1000UL;
    save();
}

void StatisticsManager::onStreakUpdate(uint16_t streak) {
    if (streak > bestStreak) {
        bestStreak = streak;
        save();
    }
}

void StatisticsManager::onWpmAchieved(uint8_t wpm) {
    if (wpm > bestWpm) {
        bestWpm = wpm;
        save();
    }
}

float StatisticsManager::trainingHours() const {
    return totalTrainingMs / 3600000.0f;
}

float StatisticsManager::accuracyPercent() const {
    if (charsSent == 0) return 0.0f;
    return (charsCorrect * 100.0f) / charsSent;
}
