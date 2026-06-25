#include "SettingsManager.h"

void SettingsManager::begin() {
    _prefs.begin(NS, false);
    load();
}

void SettingsManager::load() {
    wpm            = _prefs.getUChar("wpm", WPM_DEFAULT);
    sidetoneHz     = _prefs.getUShort("tone", SIDETONE_FREQ_DEFAULT);
    paddleMode     = static_cast<PaddleMode>(_prefs.getUChar("paddle", static_cast<uint8_t>(PaddleMode::IAMBIC_B)));
    brightness     = _prefs.getUChar("bright", 100);
    invertContacts = _prefs.getBool("invert", false);
    volume         = _prefs.getUChar("vol", 90);

    if (wpm < WPM_MIN || wpm > WPM_MAX) wpm = WPM_DEFAULT;
    if (brightness > 100) brightness = 80;
    if (volume > 100) volume = 90;
    // Migração: volumes antigos muito baixos
    if (volume < 80) {
        volume = 90;
        save();
    }
}

void SettingsManager::save() {
    _prefs.putUChar("wpm", wpm);
    _prefs.putUShort("tone", sidetoneHz);
    _prefs.putUChar("paddle", static_cast<uint8_t>(paddleMode));
    _prefs.putUChar("bright", brightness);
    _prefs.putBool("invert", invertContacts);
    _prefs.putUChar("vol", volume);
}

void SettingsManager::setWpm(uint8_t value) {
    if (value < WPM_MIN) value = WPM_MIN;
    if (value > WPM_MAX) value = WPM_MAX;
    wpm = value;
    save();
}

void SettingsManager::setSidetoneHz(uint16_t hz) {
    sidetoneHz = hz;
    save();
}

void SettingsManager::setPaddleMode(PaddleMode mode) {
    paddleMode = mode;
    save();
}

void SettingsManager::setBrightness(uint8_t value) {
    if (value > 100) value = 100;
    brightness = value;
    save();
}

void SettingsManager::setInvertContacts(bool invert) {
    invertContacts = invert;
    save();
}

void SettingsManager::setVolume(uint8_t value) {
    if (value > 100) value = 100;
    volume = value;
    save();
}

void SettingsManager::cycleSidetone() {
    static const uint16_t freqs[] = {300, 400, 500, 600, 700, 800, 900, 1000};
    constexpr size_t count = sizeof(freqs) / sizeof(freqs[0]);
    for (size_t i = 0; i < count; ++i) {
        if (freqs[i] == sidetoneHz) {
            sidetoneHz = freqs[(i + 1) % count];
            save();
            return;
        }
    }
    sidetoneHz = SIDETONE_FREQ_DEFAULT;
    save();
}

void SettingsManager::cyclePaddleMode() {
    uint8_t next = (static_cast<uint8_t>(paddleMode) + 1) % 3;
    paddleMode = static_cast<PaddleMode>(next);
    save();
}

void SettingsManager::resetToDefaults() {
    wpm = WPM_DEFAULT;
    sidetoneHz = SIDETONE_FREQ_DEFAULT;
    paddleMode = PaddleMode::IAMBIC_B;
    brightness = 100;
    invertContacts = false;
    volume = 90;
    save();
}

const char* SettingsManager::paddleModeName() const {
    switch (paddleMode) {
        case PaddleMode::IAMBIC_A:      return "Iambic A";
        case PaddleMode::IAMBIC_B:      return "Iambic B";
        case PaddleMode::STRAIGHT_KEY:  return "Straight Key";
        default:                        return "?";
    }
}

const char* SettingsManager::sidetoneName() const {
    static char buf[8];
    snprintf(buf, sizeof(buf), "%u Hz", sidetoneHz);
    return buf;
}
