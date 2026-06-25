#include "DisplayManager.h"
#include "MorseDecoder.h"
#include <cstring>

// Cores do tema
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
    d.setRotation(1);   // Paisagem 240 x 135
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

void DisplayManager::drawFooter(const char* btnA, const char* btnB) {
    auto& d = M5.Display;
    int fh = 20;
    int y = d.height() - fh;
    d.fillRect(0, y, d.width(), fh, COL_FOOTER);

    d.setTextSize(1);
    d.setTextColor(TFT_WHITE);
    d.setCursor(4, y + 5);
    d.printf("A:%s", btnA);

    int bw = strlen(btnB) * 6;
    d.setCursor(d.width() - bw - 16, y + 5);
    d.printf("B:%s", btnB);
}

void DisplayManager::drawCenteredBig(int y, const char* text, uint8_t textSize, uint32_t color) {
    auto& d = M5.Display;
    d.setTextSize(textSize);
    d.setTextColor(color);
    int tw = strlen(text) * 6 * textSize;
    d.setCursor((d.width() - tw) / 2, y);
    d.print(text);
}

void DisplayManager::drawBigText(int y, const char* text, uint8_t textSize, uint32_t color) {
    auto& d = M5.Display;
    d.setTextSize(textSize);
    d.setTextColor(color);
    d.setCursor(8, y);
    d.print(text);
}

void DisplayManager::drawCarouselItem(const char* prev, const char* current, const char* next,
                                       uint32_t color) {
    auto& d = M5.Display;
    int cy = 48;

    // Item anterior (dim)
    if (prev && prev[0]) {
        d.setTextSize(1);
        d.setTextColor(COL_DIM);
        int tw = strlen(prev) * 6;
        d.setCursor((d.width() - tw) / 2, cy);
        d.print(prev);
    }

    // Item atual — GRANDE
    d.drawRoundRect(8, cy + 16, d.width() - 16, 44, 6, color);
    d.fillRoundRect(9, cy + 17, d.width() - 18, 42, 5, 0x102040);
    drawCenteredBig(cy + 28, current ? current : "", 2, color);

    // Próximo item (dim)
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
    drawCenteredBig(72, "Telegrafia", 2, TFT_WHITE);
    drawCenteredBig(94, "Morse CW", 2, TFT_WHITE);
    drawFooter("aguarde", "...");
}

bool DisplayManager::needsRedraw() const { return _dirty; }
void DisplayManager::markDirty() { _dirty = true; }
void DisplayManager::clearDirty() { _dirty = false; }

// =============================================================================
// TELA INICIAL — 5 opções grandes
// =============================================================================
void DisplayManager::drawHome(uint8_t selectedIndex) {
    static const char* items[] = {
        "TRANSMITIR",
        "LICOES",
        "PROFESSOR",
        "CONFIG",
        "STATS"
    };
    static const char* hints[] = {
        "Monitor e Modo Livre",
        "Letras Numeros Palavras",
        "Analise do seu ritmo",
        "WPM Tom Paddle",
        "Sessoes e precisao"
    };
    constexpr uint8_t count = 5;

    if (!_dirty && _lastMode == AppMode::HOME && _lastIndex == selectedIndex) return;

    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawHeader("INICIO");

    uint8_t prev = (selectedIndex + count - 1) % count;
    uint8_t next = (selectedIndex + 1) % count;
    drawCarouselItem(items[prev], items[selectedIndex], items[next], COL_SEL);

    drawCenteredBig(118, hints[selectedIndex], 1, TFT_LIGHTGREY);
    drawFooter("prox", "ENTRAR");

    _lastMode = AppMode::HOME;
    _lastIndex = selectedIndex;
    _dirty = false;
}

// =============================================================================
// SUBMENU TRANSMITIR
// =============================================================================
void DisplayManager::drawSubMenuTransmit(uint8_t selectedIndex) {
    static const char* items[] = { "MONITOR", "MODO LIVRE", "< VOLTAR" };
    static const char* hints[] = {
        "Veja DIT DAH e texto",
        "Transmita livremente",
        "Retorna ao inicio"
    };
    constexpr uint8_t count = 3;

    if (!_dirty && _lastMode == AppMode::MENU_TRANSMIT && _lastIndex == selectedIndex) return;

    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawHeader("TRANSMITIR");

    uint8_t prev = (selectedIndex + count - 1) % count;
    uint8_t next = (selectedIndex + 1) % count;
    drawCarouselItem(items[prev], items[selectedIndex], items[next], TFT_GREEN);

    drawCenteredBig(118, hints[selectedIndex], 1, TFT_LIGHTGREY);
    drawFooter("prox", "ENTRAR");

    _lastMode = AppMode::MENU_TRANSMIT;
    _lastIndex = selectedIndex;
    _dirty = false;
}

// =============================================================================
// SUBMENU LIÇÕES
// =============================================================================
void DisplayManager::drawSubMenuLessons(uint8_t selectedIndex) {
    static const char* items[] = { "LETRAS", "NUMEROS", "PALAVRAS", "CALLSIGN", "< VOLTAR" };
    static const char* hints[] = {
        "Treino A-Z",
        "Treino 0-9",
        "CQ QSO RADIO...",
        "PY2ABC PU2XYZ...",
        "Retorna ao inicio"
    };
    constexpr uint8_t count = 5;

    if (!_dirty && _lastMode == AppMode::MENU_LESSONS && _lastIndex == selectedIndex) return;

    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawHeader("LICOES");

    uint8_t prev = (selectedIndex + count - 1) % count;
    uint8_t next = (selectedIndex + 1) % count;
    drawCarouselItem(items[prev], items[selectedIndex], items[next], TFT_ORANGE);

    drawCenteredBig(118, hints[selectedIndex], 1, TFT_LIGHTGREY);
    drawFooter("prox", "ENTRAR");

    _lastMode = AppMode::MENU_LESSONS;
    _lastIndex = selectedIndex;
    _dirty = false;
}

// =============================================================================
// TELAS DE ATIVIDADE (monitor, treino, professor, livre)
// =============================================================================
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
    const char* text = (mode == AppMode::TRAIN_FREE) ? trainer.freeText() : dec.decodedText();
    const char* feedback = trainer.feedbackText();

    char targetBuf[16] = "";
    if (mode == AppMode::TRAIN_LETTERS || mode == AppMode::TRAIN_NUMBERS) {
        snprintf(targetBuf, sizeof(targetBuf), "%c", trainer.targetChar());
    } else if (mode == AppMode::TRAIN_WORDS) {
        strncpy(targetBuf, trainer.targetWord(), sizeof(targetBuf) - 1);
    } else if (mode == AppMode::TRAIN_CALLSIGN) {
        strncpy(targetBuf, trainer.targetCallsign(), sizeof(targetBuf) - 1);
    }

    bool changed = _dirty
        || mode != _lastMode
        || settings.wpm != _lastWpm
        || speakerPlaying != _lastSpeakerPlaying
        || strcmp(seq, _lastSeq) != 0
        || strcmp(feedback, _lastFeedback) != 0
        || strcmp(targetBuf, _lastTarget) != 0
        || strncmp(text, _lastText, sizeof(_lastText) - 1) != 0;

    if (!changed) return;

    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawHeader(TrainerManager::modeName(mode), settings.wpm);

    // --- Modos de treino: alvo GRANDE no centro ---
    if (mode >= AppMode::TRAIN_LETTERS && mode <= AppMode::TRAIN_CALLSIGN) {
        bool listenMode = speakerPlaying
            && (mode == AppMode::TRAIN_LETTERS || mode == AppMode::TRAIN_NUMBERS);

        if (listenMode) {
            drawCenteredBig(28, "OUVINDO", 2, TFT_CYAN);
            drawCenteredBig(48, "no speaker", 1, TFT_LIGHTGREY);
        } else {
            drawCenteredBig(30, "TRANSMITA:", 1, TFT_LIGHTGREY);
        }

        if (mode == AppMode::TRAIN_LETTERS || mode == AppMode::TRAIN_NUMBERS) {
            char ch[2] = { trainer.targetChar(), '\0' };
            drawCenteredBig(listenMode ? 62 : 48, ch, 4, TFT_GREEN);
            char morse[16];
            MorseDecoder::charToMorse(trainer.targetChar(), morse, sizeof(morse));
            drawCenteredBig(listenMode ? 100 : 88, morse, 2, TFT_CYAN);
        } else {
            drawCenteredBig(48, targetBuf, 2, TFT_GREEN);
        }

        char scoreBuf[24];
        snprintf(scoreBuf, sizeof(scoreBuf), "Pts %u  Seq %u", trainer.score(), trainer.streak());
        drawCenteredBig(108, scoreBuf, 1, TFT_ORANGE);

        if (feedback[0]) {
            uint32_t fc = (strstr(feedback, "CORRETO")) ? COL_OK : COL_ERR;
            drawCenteredBig(122, feedback, 1, fc);
        }

        if (mode == AppMode::TRAIN_LETTERS || mode == AppMode::TRAIN_NUMBERS) {
            drawFooter("OUVIR", "INICIO");
        } else {
            drawFooter("---", "INICIO");
        }
    }
    // --- Professor ---
    else if (mode == AppMode::PROFESSOR) {
        drawCenteredBig(24, "PROFESSOR", 3, TFT_MAGENTA);

        if (feedback[0]) {
            char line1[26] = "";
            char line2[26] = "";
            const char* sp = strchr(feedback, '.');
            if (sp) {
                size_t n = sp - feedback + 1;
                if (n >= sizeof(line1)) n = sizeof(line1) - 1;
                strncpy(line1, feedback, n);
                line1[n] = '\0';
                strncpy(line2, sp + 1, sizeof(line2) - 1);
                while (line2[0] == ' ') memmove(line2, line2 + 1, strlen(line2));
            } else {
                strncpy(line1, feedback, sizeof(line1) - 1);
            }

            drawCenteredBig(48, elementName(dec.lastElement()), 3, TFT_CYAN);
            drawCenteredBig(78, line1, 2, TFT_YELLOW);
            if (line2[0]) {
                drawCenteredBig(98, line2, 2, TFT_YELLOW);
            }
        } else {
            drawCenteredBig(50, "Transmita", 2, TFT_WHITE);
            drawCenteredBig(70, "no paddle", 2, TFT_WHITE);
            drawCenteredBig(94, elementName(dec.lastElement()), 3, TFT_CYAN);
        }

        drawFooter("paddle", "INICIO");
    }
    // --- Modo Livre ---
    else if (mode == AppMode::TRAIN_FREE) {
        drawBigText(28, "Sequencia:", 1, TFT_LIGHTGREY);
        d.setTextSize(2);
        d.setTextColor(TFT_CYAN);
        d.setCursor(8, 42);
        d.print(seq[0] ? seq : "-");

        drawBigText(64, "Texto:", 1, TFT_LIGHTGREY);
        d.setTextSize(1);
        d.setTextColor(TFT_GREEN);
        d.setCursor(8, 78);
        // Última linha do texto (mais recente)
        const char* show = text;
        size_t len = strlen(text);
        if (len > 28) show = text + len - 28;
        d.print(show[0] ? show : "(vazio)");

        if (feedback[0]) {
            drawCenteredBig(108, feedback, 1, TFT_YELLOW);
        }

        drawFooter("limpar", "INICIO");
    }
    // --- Monitor (transmitir) ---
    else {
        // Elemento atual grande
        const char* el = elementName(dec.lastElement());
        drawBigText(28, "Sinal:", 1, TFT_LIGHTGREY);
        drawCenteredBig(40, el, 3, TFT_CYAN);

        // Sequência Morse grande
        drawBigText(72, "Morse:", 1, TFT_LIGHTGREY);
        drawCenteredBig(86, seq[0] ? seq : "-", 2, TFT_WHITE);

        // Último caractere decodificado
        char lastCh[2] = { dec.lastDecodedChar() ? dec.lastDecodedChar() : '_', '\0' };
        drawBigText(108, "Letra:", 1, TFT_LIGHTGREY);
        d.setTextSize(3);
        d.setTextColor(TFT_GREEN);
        d.setCursor(d.width() - 40, 104);
        d.print(lastCh);

        drawFooter("WPM+", "INICIO");
    }

    strncpy(_lastSeq, seq, sizeof(_lastSeq) - 1);
    strncpy(_lastText, text ? text : "", sizeof(_lastText) - 1);
    strncpy(_lastFeedback, feedback, sizeof(_lastFeedback) - 1);
    strncpy(_lastTarget, targetBuf, sizeof(_lastTarget) - 1);
    _lastMode = mode;
    _lastWpm = settings.wpm;
    _lastSpeakerPlaying = speakerPlaying;
    _dirty = false;
}

// =============================================================================
// CONFIGURAÇÕES — um item por vez, valor grande
// =============================================================================
void DisplayManager::drawSettings(const SettingsManager& settings, uint8_t selectedItem) {
    if (!_dirty && _lastMode == AppMode::SETTINGS && _lastSettingsItem == selectedItem) return;

    static const char* labels[] = {
        "VELOCIDADE", "SIDETONE", "PADDLE", "BRILHO", "INVERTER", "VOLUME", "RESET", "SAIR"
    };
    constexpr uint8_t count = 8;

    char valueBuf[20];
    switch (selectedItem) {
        case 0: snprintf(valueBuf, sizeof(valueBuf), "%u WPM", settings.wpm); break;
        case 1: snprintf(valueBuf, sizeof(valueBuf), "%u Hz", settings.sidetoneHz); break;
        case 2: snprintf(valueBuf, sizeof(valueBuf), "%s", settings.paddleModeName()); break;
        case 3: snprintf(valueBuf, sizeof(valueBuf), "%u%%", settings.brightness); break;
        case 4: snprintf(valueBuf, sizeof(valueBuf), "%s", settings.invertContacts ? "SIM" : "NAO"); break;
        case 5: snprintf(valueBuf, sizeof(valueBuf), "%u%%", settings.volume); break;
        default: strncpy(valueBuf, "APAGAR TUDO", sizeof(valueBuf)); break;
    }
    if (selectedItem == 7) strncpy(valueBuf, "VOLTAR", sizeof(valueBuf));

    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawHeader("CONFIG");

    // Indicador de posição
    char pos[8];
    snprintf(pos, sizeof(pos), "%u/%u", selectedItem + 1, count);
    drawCenteredBig(28, pos, 1, COL_DIM);

    drawCenteredBig(48, labels[selectedItem], 1, TFT_LIGHTGREY);
    drawCenteredBig(68, valueBuf, 3, TFT_YELLOW);

    if (selectedItem == 7) {
        drawFooter("SAIR", "SAIR");
    } else {
        drawFooter("alterar", "proximo");
    }

    _lastMode = AppMode::SETTINGS;
    _lastSettingsItem = selectedItem;
    _dirty = false;
}

// =============================================================================
// ESTATÍSTICAS — layout compacto legível
// =============================================================================
void DisplayManager::drawStatistics(const StatisticsManager& stats) {
    if (!_dirty && _lastMode == AppMode::STATISTICS) return;

    auto& d = M5.Display;
    d.fillScreen(COL_BG);
    drawHeader("ESTATISTICAS");

    char buf[32];
    int y = 30;
    d.setTextSize(1);

    auto line = [&](const char* label, const char* val) {
        d.setTextColor(TFT_LIGHTGREY);
        d.setCursor(8, y);
        d.print(label);
        d.setTextColor(TFT_WHITE);
        d.setCursor(100, y);
        d.print(val);
        y += 14;
    };

    snprintf(buf, sizeof(buf), "%lu", (unsigned long)stats.totalSessions);
    line("Sessoes:", buf);

    snprintf(buf, sizeof(buf), "%.1f h", stats.trainingHours());
    line("Horas:", buf);

    snprintf(buf, sizeof(buf), "%.0f%%", stats.accuracyPercent());
    line("Precisao:", buf);

    snprintf(buf, sizeof(buf), "%lu", (unsigned long)stats.charsCorrect);
    line("Acertos:", buf);

    snprintf(buf, sizeof(buf), "%lu", (unsigned long)stats.charsIncorrect);
    line("Erros:", buf);

    snprintf(buf, sizeof(buf), "%u", stats.bestStreak);
    line("Melhor seq:", buf);

    snprintf(buf, sizeof(buf), "%u", stats.bestWpm);
    line("Melhor WPM:", buf);

    drawFooter("---", "INICIO");

    _lastMode = AppMode::STATISTICS;
    _dirty = false;
}
