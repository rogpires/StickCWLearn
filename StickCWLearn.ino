// =============================================================================
// StickCWLearn — Treinador CW para M5StickS3
// =============================================================================
// NAVEGAÇÃO (sempre igual):
//   MENU (início, config):  A = PROX   |  B = OK
//   PRÁTICA (treino, monitor): A = OUVIR |  B = MENU (volta)
// =============================================================================

#include <M5Unified.h>
#include "Config.h"
#include "CWInput.h"
#include "CWGenerator.h"
#include "TrainerManager.h"
#include "DisplayManager.h"
#include "SettingsManager.h"
#include "StatisticsManager.h"

SettingsManager   gSettings;
StatisticsManager gStats;
CWInput           gInput;
CWGenerator       gSidetone;
TrainerManager    gTrainer;
DisplayManager    gDisplay;

AppMode  gAppMode       = AppMode::HOME;
uint8_t  gMenuIndex     = 0;
uint8_t  gSettingsItem  = 0;
uint32_t gLastDisplayMs = 0;
uint32_t gBootMs        = 0;
bool     gBootDone      = false;
bool     gWasPlaying    = false;

static const AppMode HOME_MODES[] = {
    AppMode::TRAIN_LETTERS, AppMode::TRAIN_NUMBERS, AppMode::TRAIN_WORDS,
    AppMode::TRAIN_CALLSIGN, AppMode::MONITOR, AppMode::SETTINGS
};
static constexpr uint8_t HOME_COUNT = 6;
static constexpr uint8_t SETTINGS_COUNT = 5;

void onKeyEvent(const KeyEvent& ev);
void handleButtons();
void goHome();
void enterMode(AppMode mode);
void applySettings();
void startPendingPlayback();
bool isPracticeMode(AppMode m);

void setup() {
    auto cfg = M5.config();
    cfg.internal_mic = false;
    cfg.internal_spk = true;
    M5.begin(cfg);

    gSettings.begin();
    gStats.begin();
    gInput.begin();
    gSidetone.begin();
    gTrainer.begin(&gStats);
    gDisplay.begin();
    applySettings();

    gDisplay.drawBootScreen();
    gBootMs = millis();
    gInput.setCallback(onKeyEvent);
    gAppMode = AppMode::HOME;
    gMenuIndex = 0;
    gStats.onSessionStart();
    gBootDone = false;
    gDisplay.markDirty();
}

void loop() {
    M5.update();
    uint32_t now = millis();

    if (!gBootDone) {
        if (now - gBootMs < 1200) return;
        gBootDone = true;
        gDisplay.markDirty();
    }

    gInput.configure(gSettings);
    gInput.update(now);
    gTrainer.setWpm(gSettings.wpm);
    gTrainer.update(now);
    gSidetone.update(now);

    bool playing = gSidetone.isPlaying();
    if (gWasPlaying && !playing) {
        gTrainer.onPlaybackFinished();
        gDisplay.markDirty();
    }
    gWasPlaying = playing;

    startPendingPlayback();
    handleButtons();

    if (now - gLastDisplayMs >= DISPLAY_REFRESH_MS || gDisplay.needsRedraw()) {
        gLastDisplayMs = now;
        if (gAppMode == AppMode::HOME) {
            gDisplay.drawHome(gMenuIndex);
        } else if (gAppMode == AppMode::SETTINGS) {
            gDisplay.drawSettings(gSettings, gSettingsItem);
        } else {
            gDisplay.drawActivity(gTrainer, gSettings, playing);
        }
    }
}

void startPendingPlayback() {
    if (!gTrainer.pendingPlayback() || gSidetone.isPlaying()) return;

    if (gTrainer.playbackIsText()) {
        gSidetone.playText(gTrainer.playbackText(), gSettings.wpm);
    } else {
        gSidetone.playCharacter(gTrainer.playbackChar(), gSettings.wpm);
    }
    gTrainer.clearPlaybackPending();
    gDisplay.markDirty();
}

void onKeyEvent(const KeyEvent& ev) {
    switch (ev.type) {
        case KeyEventType::TONE_ON:
            gSidetone.stopPlayback();
            gSidetone.toneOn();
            break;
        case KeyEventType::TONE_OFF:
            gSidetone.toneOff();
            break;
        case KeyEventType::ELEMENT_COMPLETE:
            gTrainer.onKeyEvent(ev);
            gStats.onWpmAchieved(gSettings.wpm);
            gDisplay.markDirty();
            break;
    }
}

bool isPracticeMode(AppMode m) {
    return m == AppMode::MONITOR
        || m == AppMode::TRAIN_LETTERS || m == AppMode::TRAIN_NUMBERS
        || m == AppMode::TRAIN_WORDS || m == AppMode::TRAIN_CALLSIGN;
}

void goHome() {
    gSidetone.stopPlayback();
    gAppMode = AppMode::HOME;
    gMenuIndex = 0;
    gDisplay.markDirty();
}

void enterMode(AppMode mode) {
    gAppMode = mode;
    if (mode == AppMode::SETTINGS) {
        gSettingsItem = 0;
    } else {
        gTrainer.setMode(mode);
    }
    gDisplay.markDirty();
}

void handleButtons() {
    // ===== BtnA =====
    if (M5.BtnA.wasPressed()) {
        if (gAppMode == AppMode::HOME) {
            gMenuIndex = (gMenuIndex + 1) % HOME_COUNT;
            gDisplay.markDirty();
        }
        else if (gAppMode == AppMode::SETTINGS) {
            if (gSettingsItem >= SETTINGS_COUNT - 1) {
                goHome();
            } else {
                switch (gSettingsItem) {
                    case 0: gSettings.setWpm(gSettings.wpm >= WPM_MAX ? WPM_MIN : gSettings.wpm + 1); break;
                    case 1: gSettings.cycleSidetone(); break;
                    case 2: gSettings.cyclePaddleMode(); break;
                    case 3: gSettings.setVolume(gSettings.volume >= 100 ? 10 : gSettings.volume + 10); break;
                }
                applySettings();
                gDisplay.markDirty();
            }
        }
        else if (isPracticeMode(gAppMode)) {
            gTrainer.replayTarget();
            gDisplay.markDirty();
        }
    }

    // ===== BtnB =====
    if (M5.BtnB.wasPressed()) {
        if (gAppMode == AppMode::HOME) {
            enterMode(HOME_MODES[gMenuIndex]);
        }
        else if (gAppMode == AppMode::SETTINGS) {
            if (gSettingsItem >= SETTINGS_COUNT - 1) {
                goHome();
            } else {
                gSettingsItem++;
                gDisplay.markDirty();
            }
        }
        else if (isPracticeMode(gAppMode)) {
            goHome();
        }
    }
}

void applySettings() {
    gDisplay.setBrightness(gSettings.brightness);
    gSidetone.configure(gSettings.sidetoneHz, gSettings.volume);
    gTrainer.setWpm(gSettings.wpm);
}
