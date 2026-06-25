// =============================================================================
// StickCWLearn.ino — Professor Inteligente de Telegrafia Morse
// =============================================================================
//
// NAVEGAÇÃO UNIFICADA (M5StickS3 — BtnA GPIO11, BtnB GPIO12):
//   MENUS (INICIO / submenus):  A = próximo   |  B = ENTRAR
//   ATIVIDADES (treino etc.):   A = ação extra |  B = INÍCIO
//   CONFIG:                     A = alterar    |  B = próximo (último = SAIR)
//   NÃO use o botão de ENERGIA — ele reinicia o aparelho!
// =============================================================================

#include <M5Unified.h>
#include "Config.h"
#include "CWInput.h"
#include "CWGenerator.h"
#include "MorseDecoder.h"
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

AppMode  gAppMode      = AppMode::HOME;
uint8_t  gMenuIndex    = 0;
uint8_t  gSettingsItem = 0;
uint32_t gLastDisplayMs = 0;
uint32_t gBootMs        = 0;
bool     gBootDone      = false;
bool     gWasSpeakerPlaying = false;

static const AppMode TRANSMIT_MODES[] = { AppMode::MONITOR, AppMode::TRAIN_FREE };
static constexpr uint8_t TRANSMIT_COUNT = 3;

static const AppMode LESSON_MODES[] = {
    AppMode::TRAIN_LETTERS, AppMode::TRAIN_NUMBERS,
    AppMode::TRAIN_WORDS, AppMode::TRAIN_CALLSIGN
};
static constexpr uint8_t LESSON_COUNT = 5;

static constexpr uint8_t HOME_COUNT = 5;
static constexpr uint8_t SETTINGS_COUNT = 8;

void onKeyEvent(const KeyEvent& ev);
void handleButtons();
void handleMenuConfirm();
void handleBack();
void goHome();
void enterActivity(AppMode mode);
void applySettings();
bool isActivityMode(AppMode m);
bool isMenuMode(AppMode m);

// =============================================================================
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

// =============================================================================
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

    bool speakerPlaying = gSidetone.isPlaying();
    if (gWasSpeakerPlaying && !speakerPlaying) {
        gDisplay.markDirty();
    }
    gWasSpeakerPlaying = speakerPlaying;

    if (gTrainer.pendingMorsePlayback() && !speakerPlaying) {
        gSidetone.playCharacter(gTrainer.playbackChar(), gSettings.wpm);
        gTrainer.clearPlaybackPending();
        gDisplay.markDirty();
    }

    if (gAppMode == AppMode::PROFESSOR) {
        gTrainer.analyzeManipulation(gInput, gSettings.wpm);
    }

    handleButtons();

    if (now - gLastDisplayMs >= DISPLAY_REFRESH_MS || gDisplay.needsRedraw()) {
        gLastDisplayMs = now;
        switch (gAppMode) {
            case AppMode::HOME:
                gDisplay.drawHome(gMenuIndex);
                break;
            case AppMode::MENU_TRANSMIT:
                gDisplay.drawSubMenuTransmit(gMenuIndex);
                break;
            case AppMode::MENU_LESSONS:
                gDisplay.drawSubMenuLessons(gMenuIndex);
                break;
            case AppMode::SETTINGS:
                gDisplay.drawSettings(gSettings, gSettingsItem);
                break;
            case AppMode::STATISTICS:
                gDisplay.drawStatistics(gStats);
                break;
            default:
                gDisplay.drawActivity(gTrainer, gSettings, speakerPlaying);
                break;
        }
    }
}

// =============================================================================
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

// =============================================================================
bool isActivityMode(AppMode m) {
    return m == AppMode::MONITOR || m == AppMode::TRAIN_FREE
        || m == AppMode::TRAIN_LETTERS || m == AppMode::TRAIN_NUMBERS
        || m == AppMode::TRAIN_WORDS || m == AppMode::TRAIN_CALLSIGN
        || m == AppMode::PROFESSOR;
}

bool isMenuMode(AppMode m) {
    return m == AppMode::HOME || m == AppMode::MENU_TRANSMIT || m == AppMode::MENU_LESSONS;
}

void goHome() {
    if (gAppMode == AppMode::TRAIN_FREE) {
        gTrainer.saveFreeSession();
    }
    gSidetone.stopPlayback();
    gAppMode = AppMode::HOME;
    gMenuIndex = 0;
    gDisplay.markDirty();
}

void enterActivity(AppMode mode) {
    gAppMode = mode;
    gTrainer.setMode(mode);
    gDisplay.markDirty();
}

void handleBack() {
    goHome();
}

// =============================================================================
void handleMenuConfirm() {
    switch (gAppMode) {
        case AppMode::HOME:
            switch (gMenuIndex) {
                case 0:
                    gAppMode = AppMode::MENU_TRANSMIT;
                    gMenuIndex = 0;
                    gDisplay.markDirty();
                    break;
                case 1:
                    gAppMode = AppMode::MENU_LESSONS;
                    gMenuIndex = 0;
                    gDisplay.markDirty();
                    break;
                case 2:
                    enterActivity(AppMode::PROFESSOR);
                    break;
                case 3:
                    gAppMode = AppMode::SETTINGS;
                    gSettingsItem = 0;
                    gDisplay.markDirty();
                    break;
                case 4:
                    gAppMode = AppMode::STATISTICS;
                    gDisplay.markDirty();
                    break;
            }
            break;

        case AppMode::MENU_TRANSMIT:
            if (gMenuIndex == TRANSMIT_COUNT - 1) goHome();
            else enterActivity(TRANSMIT_MODES[gMenuIndex]);
            break;

        case AppMode::MENU_LESSONS:
            if (gMenuIndex == LESSON_COUNT - 1) goHome();
            else enterActivity(LESSON_MODES[gMenuIndex]);
            break;

        default:
            break;
    }
}

// =============================================================================
void handleButtons() {
    // ---- BtnA: próximo (menus) / ação contextual (atividades) ----
    if (M5.BtnA.wasPressed()) {
        switch (gAppMode) {
            case AppMode::HOME:
                gMenuIndex = (gMenuIndex + 1) % HOME_COUNT;
                gDisplay.markDirty();
                break;

            case AppMode::MENU_TRANSMIT:
                gMenuIndex = (gMenuIndex + 1) % TRANSMIT_COUNT;
                gDisplay.markDirty();
                break;

            case AppMode::MENU_LESSONS:
                gMenuIndex = (gMenuIndex + 1) % LESSON_COUNT;
                gDisplay.markDirty();
                break;

            case AppMode::SETTINGS:
                if (gSettingsItem == 7) {
                    goHome();
                    break;
                }
                switch (gSettingsItem) {
                    case 0:
                        gSettings.setWpm(gSettings.wpm >= WPM_MAX ? WPM_MIN : gSettings.wpm + 1);
                        break;
                    case 1: gSettings.cycleSidetone(); break;
                    case 2: gSettings.cyclePaddleMode(); break;
                    case 3: gSettings.setBrightness(gSettings.brightness >= 100 ? 10 : gSettings.brightness + 10); break;
                    case 4: gSettings.setInvertContacts(!gSettings.invertContacts); break;
                    case 5: gSettings.setVolume(gSettings.volume >= 100 ? 10 : gSettings.volume + 10); break;
                    case 6: gStats.clearAll(); break;
                }
                applySettings();
                gDisplay.markDirty();
                break;

            case AppMode::MONITOR:
                gSettings.setWpm(gSettings.wpm >= WPM_MAX ? WPM_MIN : gSettings.wpm + 1);
                applySettings();
                gDisplay.markDirty();
                break;

            case AppMode::TRAIN_FREE:
                gTrainer.clearFreeText();
                gDisplay.markDirty();
                break;

            case AppMode::TRAIN_LETTERS:
            case AppMode::TRAIN_NUMBERS:
                gTrainer.replayTarget();
                gDisplay.markDirty();
                break;

            // PROFESSOR, PALAVRAS, CALLSIGN: BtnA sem ação
            default:
                break;
        }
    }

    // ---- BtnB: ENTRAR (menus) / INÍCIO ou próximo (config) ----
    if (M5.BtnB.wasPressed()) {
        switch (gAppMode) {
            case AppMode::HOME:
            case AppMode::MENU_TRANSMIT:
            case AppMode::MENU_LESSONS:
                handleMenuConfirm();
                break;

            case AppMode::SETTINGS:
                if (gSettingsItem >= SETTINGS_COUNT - 1) {
                    goHome();
                } else {
                    gSettingsItem++;
                    gDisplay.markDirty();
                }
                break;

            case AppMode::STATISTICS:
                goHome();
                break;

            default:
                if (isActivityMode(gAppMode)) {
                    handleBack();
                }
                break;
        }
    }

    // BtnA longo em menus = ENTRAR (alternativa)
    if (M5.BtnA.pressedFor(500) && isMenuMode(gAppMode)) {
        handleMenuConfirm();
        while (M5.BtnA.isPressed()) { M5.update(); }
    }
}

// =============================================================================
void applySettings() {
    gDisplay.setBrightness(gSettings.brightness);
    gSidetone.configure(gSettings.sidetoneHz, gSettings.volume);
    gTrainer.setWpm(gSettings.wpm);
}
