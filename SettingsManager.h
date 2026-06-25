#pragma once

#include "Config.h"
#include <Preferences.h>

// Gerencia configurações persistentes em Flash (Preferences)
class SettingsManager {
public:
    void begin();
    void load();
    void save();

    uint8_t  wpm;
    uint16_t sidetoneHz;
    PaddleMode paddleMode;
    uint8_t  brightness;
    bool     invertContacts;
    uint8_t  volume;          // 0-100

    void setWpm(uint8_t value);
    void setSidetoneHz(uint16_t hz);
    void setPaddleMode(PaddleMode mode);
    void setBrightness(uint8_t value);
    void setInvertContacts(bool invert);
    void setVolume(uint8_t value);

    void cycleSidetone();
    void cyclePaddleMode();
    void resetToDefaults();

    const char* paddleModeName() const;
    const char* sidetoneName() const;

private:
    Preferences _prefs;
    static constexpr const char* NS = "cwset";
};
