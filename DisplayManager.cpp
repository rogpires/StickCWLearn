#include "DisplayManager.h"
#include "MorseDecoder.h"
#include <cstring>

static constexpr uint32_t COL_BG       = TFT_BLACK;
static constexpr uint32_t COL_HEADER = 0x0010AA;
static constexpr uint32_t COL_FOOTER = 0x0010AA;
static constexpr uint32_t COL_SEL    = TFT_YELLOW;
static constexpr uint32_t COL_DIM    = 0x528A;
static constexpr uint32_t COL_OK     = TFT_GREEN;
static constexpr uint32_t COL_ERR    = TFT_RED;

void DisplayManager::begin() {
    _dirty = true;
    _lastMode = AppMode::HOME;
    _lastIndex = 255;
    _lastSettingsItem = 255;
    _lastSeq[0] = _lastText[0] = _lastFeedback[0] = _lastTarget[0] = '\0';
    _lastWpm = 0;
    _lastSpeakerPlaying = false;
    auto& d = M5.Display;
    d.setRotation(1);
    d.setTextWrap(false);
    d.fillScreen(COL_BG);
}

void DisplayManager::setBrightness(uint8_t percent) {
    if (percent > 100) percent = 100;
    M5.Display.setBrightness(map(percent, 0, 100, 40, 255));
}

void DisplayManager::drawHeader(const char* title, uint8_t wpm) {
    auto& d = M5.Display;
    d.fillRect(0, 0, d.width(), 22, COL_HEADER);
    d.setTextSize(1);
    d.setTextColor(TFT_WHITE);
    d.setCursor(6, 6);
    d.print(title);
    if (wpm > 0) {
        char buf[12];
        snprintf(buf, sizeof(buf), "%u WPM", wpm);
        d.setTextColor(TFT_CYAN);
        d.setCursor(d.width() - 56, 6);
        d.print(buf);
    }
}

void DisplayManager::drawFooterMenu() {
    auto& d = M5.Display;
    int y = d.height() - 20;
    d.fillRect(0, y, d.width(), 20, COL_FOOTER);
    d.setTextSize(1);
    d.setTextColor(TFT_WHITE);
    d.setCursor(4, y + 5);
    d.print("A:PROX  B:OK");
}

void DisplayManager::drawFooterPractice() {
    auto& d = M5.Display;
    int y = d.height() - 20;
    d.fillRect(0, y, d.width(), 20, COL_FOOTER);
    d.setTextSize(1);
    d.setTextColor(TFT_WHITE);
    d.setCursor(4, y + 5);
    d.print("A:OUVIR  B:MENU");
}

void DisplayManager::drawCenteredBig(int y, const char* text, uint8_t textSize, uint32_t color) {
    auto& d = M5.Display;
    d.setTextSize(textSize);
    d.setTextColor(color);
    int tw = strlen(text) * 6 * textSize;
    d.setCursor((d.width() - tw) / 2, y);
    d.print(text);
}

void DisplayManager::drawCarouselItem(const char* prev, const char* current, const char* next,
                                       uint32_t color) {
    auto& d = M5.Display;
    int cy = 48;
    if (prev && prev[0]) {
        d.setTextSize(1);
        d.setTextColor(COL_DIM);
        int tw = strlen(prev) * 6;
        d.setCursor((d.width() - tw) / 2, cy);
        d.print(prev);
    }
    d.drawRoundRect(8, cy + 16, d.width() - 16, 44, 6, color);
    d.fillRoundRect(9, cy + 17, d.width() - 18, 42, 5, 0x102040);
    drawCenteredBig(cy + 28, current ? current : "", 2, color);
    if (next && next[0]) {
        d.setTextSize(1);
        d.setTextColor(COL_DIM);
        int tw = strlen(next) * 6;
        d.setCursor((d.width() - tw) / 2, cy + 68);
        d.print(next);
    }
}

void DisplayManager::drawBootScreen() {
    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawCenteredBig(40, "CW TEACHER", 3, TFT_CYAN);
    drawCenteredBig(72, "Pratica CW", 2, TFT_WHITE);
    drawFooterMenu();
}

bool DisplayManager::needsRedraw() const { return _dirty; }
void DisplayManager::markDirty() { _dirty = true; }
void DisplayManager::clearDirty() { _dirty = false; }

// Menu principal — um único nível, foco em prática
void DisplayManager::drawHome(uint8_t selectedIndex) {
    static const char* items[] = {
        "LETRAS", "NUMEROS", "PALAVRAS", "CALLSIGN", "MONITOR", "CONFIG"
    };
    static const char* hints[] = {
        "Ouvir e repetir A-Z",
        "Ouvir e repetir 0-9",
        "Palavras de radio",
        "Indicativos BR",
        "Ver o que transmite",
        "WPM tom volume"
    };
    constexpr uint8_t count = 6;

    if (!_dirty && _lastMode == AppMode::HOME && _lastIndex == selectedIndex) return;

    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawHeader("CW PRATICA");

    uint8_t prev = (selectedIndex + count - 1) % count;
    uint8_t next = (selectedIndex + 1) % count;
    drawCarouselItem(items[prev], items[selectedIndex], items[next], COL_SEL);
    drawCenteredBig(118, hints[selectedIndex], 1, TFT_LIGHTGREY);
    drawFooterMenu();

    _lastMode = AppMode::HOME;
    _lastIndex = selectedIndex;
    _dirty = false;
}

const char* DisplayManager::elementName(MorseElement el) const {
    switch (el) {
        case MorseElement::DIT: return "DIT";
        case MorseElement::DAH: return "DAH";
        default:                return "-";
    }
}

void DisplayManager::drawActivity(const TrainerManager& trainer, const SettingsManager& settings,
                                   bool speakerPlaying) {
    const auto& dec = trainer.decoder();
    AppMode mode = trainer.mode();
    const char* seq = dec.currentSequence();
    const char* feedback = trainer.feedbackText();
    bool listening = speakerPlaying;

    char targetBuf[16] = "";
    if (mode == AppMode::TRAIN_LETTERS || mode == AppMode::TRAIN_NUMBERS) {
        snprintf(targetBuf, sizeof(targetBuf), "%c", trainer.targetChar());
    } else if (mode == AppMode::TRAIN_WORDS) {
        strncpy(targetBuf, trainer.targetWord(), sizeof(targetBuf) - 1);
    } else if (mode == AppMode::TRAIN_CALLSIGN) {
        strncpy(targetBuf, trainer.targetCallsign(), sizeof(targetBuf) - 1);
    }

    bool changed = _dirty || mode != _lastMode || settings.wpm != _lastWpm
        || speakerPlaying != _lastSpeakerPlaying
        || strcmp(seq, _lastSeq) != 0 || strcmp(feedback, _lastFeedback) != 0
        || strcmp(targetBuf, _lastTarget) != 0;
    if (!changed) return;

    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawHeader(TrainerManager::modeName(mode), settings.wpm);

    if (mode >= AppMode::TRAIN_LETTERS && mode <= AppMode::TRAIN_CALLSIGN) {
        if (listening) {
            drawCenteredBig(28, "OUVINDO", 3, TFT_CYAN);
        } else {
            drawCenteredBig(28, "TRANSMITA", 2, TFT_GREEN);
        }

        if (mode == AppMode::TRAIN_LETTERS || mode == AppMode::TRAIN_NUMBERS) {
            char ch[2] = { trainer.targetChar(), '\0' };
            drawCenteredBig(56, ch, 4, TFT_YELLOW);
            char morse[16];
            MorseDecoder::charToMorse(trainer.targetChar(), morse, sizeof(morse));
            drawCenteredBig(92, morse, 2, TFT_WHITE);
        } else {
            drawCenteredBig(56, targetBuf, 2, TFT_YELLOW);
            char prog[20];
            snprintf(prog, sizeof(prog), "letra %u de %u",
                (unsigned)(trainer.targetProgress() + 1),
                (unsigned)strlen(targetBuf));
            drawCenteredBig(88, prog, 1, TFT_LIGHTGREY);
        }

        char scoreBuf[24];
        snprintf(scoreBuf, sizeof(scoreBuf), "Acertos: %u", trainer.score());
        drawCenteredBig(112, scoreBuf, 1, TFT_ORANGE);

        if (feedback[0]) {
            uint32_t fc = (strstr(feedback, "OK")) ? COL_OK : COL_ERR;
            drawCenteredBig(124, feedback, 1, fc);
        }
    }
    else if (mode == AppMode::MONITOR) {
        drawCenteredBig(36, elementName(dec.lastElement()), 3, TFT_CYAN);
        drawCenteredBig(72, seq[0] ? seq : "-", 2, TFT_WHITE);
        char ch[2] = { dec.lastDecodedChar() ? dec.lastDecodedChar() : '_', '\0' };
        drawCenteredBig(100, ch, 3, TFT_GREEN);
    }

    drawFooterPractice();

    strncpy(_lastSeq, seq, sizeof(_lastSeq) - 1);
    strncpy(_lastFeedback, feedback, sizeof(_lastFeedback) - 1);
    strncpy(_lastTarget, targetBuf, sizeof(_lastTarget) - 1);
    _lastMode = mode;
    _lastWpm = settings.wpm;
    _lastSpeakerPlaying = speakerPlaying;
    _dirty = false;
}

void DisplayManager::drawSettings(const SettingsManager& settings, uint8_t selectedItem) {
    if (!_dirty && _lastMode == AppMode::SETTINGS && _lastSettingsItem == selectedItem) return;

    static const char* labels[] = { "WPM", "TOM Hz", "PADDLE", "VOLUME", "SAIR" };
    constexpr uint8_t count = 5;

    char valueBuf[20];
    switch (selectedItem) {
        case 0: snprintf(valueBuf, sizeof(valueBuf), "%u", settings.wpm); break;
        case 1: snprintf(valueBuf, sizeof(valueBuf), "%u", settings.sidetoneHz); break;
        case 2: snprintf(valueBuf, sizeof(valueBuf), "%s", settings.paddleModeName()); break;
        case 3: snprintf(valueBuf, sizeof(valueBuf), "%u%%", settings.volume); break;
        default: strncpy(valueBuf, "VOLTAR", sizeof(valueBuf)); break;
    }

    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawHeader("CONFIG");

    char pos[8];
    snprintf(pos, sizeof(pos), "%u/%u", selectedItem + 1, count);
    drawCenteredBig(28, pos, 1, COL_DIM);
    drawCenteredBig(48, labels[selectedItem], 1, TFT_LIGHTGREY);
    drawCenteredBig(68, valueBuf, 3, TFT_YELLOW);
    drawFooterMenu();

    _lastMode = AppMode::SETTINGS;
    _lastSettingsItem = selectedItem;
    _dirty = false;
}
