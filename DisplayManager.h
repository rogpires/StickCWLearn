#pragma once

#include "Config.h"
#include "TrainerManager.h"
#include "SettingsManager.h"
#include "StatisticsManager.h"
#include <M5Unified.h>

class DisplayManager {
public:
    void begin();
    void setBrightness(uint8_t percent);

    void drawBootScreen();
    void drawHome(uint8_t selectedIndex);
    void drawActivity(const TrainerManager& trainer, const SettingsManager& settings,
                      bool speakerPlaying);
    void drawSettings(const SettingsManager& settings, uint8_t selectedItem);

    bool needsRedraw() const;
    void markDirty();
    void clearDirty();

private:
    bool _dirty;
    AppMode _lastMode;
    uint8_t _lastIndex;
    uint8_t _lastSettingsItem;
    char _lastSeq[16];
    char _lastText[48];
    char _lastFeedback[64];
    uint8_t _lastWpm;
    char _lastTarget[16];
    bool _lastSpeakerPlaying;

    void drawHeader(const char* title, uint8_t wpm = 0);
    void drawFooterMenu();
    void drawFooterPractice();
    void drawCenteredBig(int y, const char* text, uint8_t textSize, uint32_t color);
    void drawCarouselItem(const char* prev, const char* current, const char* next,
                          uint32_t color = TFT_CYAN);
    const char* elementName(MorseElement el) const;
};
