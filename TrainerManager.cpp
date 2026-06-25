#include "TrainerManager.h"
#include <cstring>
#include <cstdlib>

const char* TrainerManager::WORD_BANK[] = {
    "RADIO", "MORSE", "ANTENA", "CQ", "QSO", "QTH",
    "ARDUINO", "ESP32", "TELEGRAFIA", "PADDLE"
};

void TrainerManager::begin(StatisticsManager* stats) {
    _stats = stats;
    _mode = AppMode::MONITOR;
    _score = _streak = _attempts = 0;
    _awaiting = true;
    _evaluated = false;
    _inputMorseLen = 0;
    _freeLen = 0;
    _freeText[0] = '\0';
    _status[0] = _feedback[0] = '\0';
    _decoder.reset();
    _prefs.begin("cwsess", false);
    _playTargetPending = false;
    _playTargetChar = '\0';
    setStatus("PRONTO");
}

void TrainerManager::setMode(AppMode mode) {
    _mode = mode;
    resetTrainingRound();
    _decoder.reset();

    switch (mode) {
        case AppMode::MONITOR:
            setStatus("PRONTO");
            setFeedback("");
            break;
        case AppMode::TRAIN_LETTERS:
            pickRandomLetter();
            setStatus("OUVINDO");
            setFeedback("Ouça no speaker");
            break;
        case AppMode::TRAIN_NUMBERS:
            pickRandomNumber();
            setStatus("OUVINDO");
            setFeedback("Ouça no speaker");
            break;
        case AppMode::TRAIN_WORDS:
            pickRandomWord();
            setStatus("TRANSMITA");
            break;
        case AppMode::TRAIN_CALLSIGN:
            generateCallsign();
            setStatus("TRANSMITA");
            break;
        case AppMode::TRAIN_FREE:
            loadLastFreeSession();
            setStatus("LIVRE");
            setFeedback("");
            break;
        case AppMode::PROFESSOR:
            setStatus("ANALISANDO");
            setFeedback("Transmita para analise.");
            break;
        default:
            break;
    }
}

AppMode TrainerManager::mode() const {
    return _mode;
}

void TrainerManager::onKeyEvent(const KeyEvent& ev) {
    if (ev.type == KeyEventType::ELEMENT_COMPLETE) {
        _decoder.onElement(ev.element);
    }
}

void TrainerManager::update(uint32_t nowMs) {
    _decoder.update(nowMs);

    if (!_decoder.hasNewChar()) return;

    char c = _decoder.lastDecodedChar();
    _decoder.clearNewCharFlag();

    switch (_mode) {
        case AppMode::MONITOR:
        case AppMode::PROFESSOR:
            break;

        case AppMode::TRAIN_LETTERS:
        case AppMode::TRAIN_NUMBERS:
            evaluateCharacter(c);
            break;

        case AppMode::TRAIN_WORDS:
        case AppMode::TRAIN_CALLSIGN: {
            const char* target = (_mode == AppMode::TRAIN_WORDS) ? _targetWord : _targetCallsign;
            if (_wordCharIndex < strlen(target)) {
                char expected = target[_wordCharIndex];
                bool ok = (c == expected);
                _attempts++;
                if (_stats) _stats->onCharacterSent(ok);
                if (ok) {
                    _score++;
                    _streak++;
                    if (_stats) _stats->onStreakUpdate(_streak);
                    setFeedback("OK CORRETO");
                } else {
                    _streak = 0;
                    char buf[48];
                    snprintf(buf, sizeof(buf), "X INCORRETO | %c", expected);
                    setFeedback(buf);
                }
                _wordCharIndex++;
                if (_wordCharIndex >= strlen(target)) {
                    if (_wordCharIndex == strlen(target) && _score > 0) {
                        setStatus("RODADA OK");
                        char fb[48];
                        snprintf(fb, sizeof(fb), "Palavra: %s", target);
                        setFeedback(fb);
                    }
                    if (_mode == AppMode::TRAIN_WORDS) pickRandomWord();
                    else generateCallsign();
                    _wordCharIndex = 0;
                    setStatus("TRANSMITA");
                }
            }
            break;
        }

        case AppMode::TRAIN_FREE:
            if (_freeLen < SESSION_MAX_CHARS) {
                _freeText[_freeLen++] = c;
                _freeText[_freeLen] = '\0';
            }
            if (_decoder.wordJustEnded()) {
                if (_freeLen < SESSION_MAX_CHARS - 1) {
                    _freeText[_freeLen++] = ' ';
                    _freeText[_freeLen] = '\0';
                }
                _decoder.clearWordFlag();
            }
            break;

        default:
            break;
    }
}

const char* TrainerManager::statusText() const {
    return _status;
}

const char* TrainerManager::feedbackText() const {
    return _feedback;
}

char TrainerManager::targetChar() const {
    return _targetChar;
}

const char* TrainerManager::targetWord() const {
    return _targetWord;
}

const char* TrainerManager::targetCallsign() const {
    return _targetCallsign;
}

uint16_t TrainerManager::score() const {
    return _score;
}

uint16_t TrainerManager::streak() const {
    return _streak;
}

uint16_t TrainerManager::totalAttempts() const {
    return _attempts;
}

bool TrainerManager::awaitingInput() const {
    return _awaiting;
}

void TrainerManager::clearFreeText() {
    _freeLen = 0;
    _freeText[0] = '\0';
    _decoder.reset();
    setFeedback("Texto limpo.");
}

void TrainerManager::saveFreeSession() {
    _prefs.putString("text", _freeText);
    _prefs.putUInt("len", _freeLen);
    setFeedback("Sessao salva!");
}

void TrainerManager::loadLastFreeSession() {
    String saved = _prefs.getString("text", "");
    if (saved.length() > 0) {
        strncpy(_freeText, saved.c_str(), SESSION_MAX_CHARS);
        _freeText[SESSION_MAX_CHARS] = '\0';
        _freeLen = strlen(_freeText);
        setFeedback("Ultima sessao carregada.");
    }
}

const char* TrainerManager::freeText() const {
    return (_mode == AppMode::TRAIN_FREE) ? _freeText : _decoder.decodedText();
}

void TrainerManager::analyzeManipulation(const CWInput& input, uint8_t wpm) {
    if (_mode != AppMode::PROFESSOR) return;

    uint8_t count = input.historyCount();
    if (count == 0) {
        setFeedback("Aguardando transmissao...");
        return;
    }

    const CWInput::ElementRecord* rec = input.historyAt(0);
    if (!rec) return;

    uint32_t ditMs = ditDurationMs(wpm);
    float ratio = (float)rec->durationMs / ditMs;

    if (rec->type == MorseElement::DIT) {
        if (ratio < PROF_DIT_MIN_RATIO) {
            setFeedback("Seu ponto esta muito curto.");
        } else if (ratio > PROF_DIT_MAX_RATIO) {
            setFeedback("Seu ponto esta muito longo.");
        } else if (rec->gapAfterMs > 0) {
            float gapRatio = (float)rec->gapAfterMs / ditMs;
            if (gapRatio < PROF_LETTER_GAP_MIN && gapRatio > 1.5f) {
                setFeedback("Espacamento entre letras insuficiente.");
            } else {
                setFeedback("Excelente ritmo.");
            }
        } else {
            setFeedback("Bom ponto! Continue assim.");
        }
    } else if (rec->type == MorseElement::DAH) {
        if (ratio < PROF_DAH_MIN_RATIO) {
            setFeedback("Seu traco esta muito curto.");
        } else if (ratio > PROF_DAH_MAX_RATIO) {
            setFeedback("Seu traco esta muito longo.");
        } else {
            setFeedback("Bom traco! Ritmo consistente.");
        }
    }

    // Análise de consistência nos últimos 5 elementos
    if (count >= 3) {
        float sum = 0;
        uint8_t n = 0;
        for (uint8_t i = 0; i < count && i < 5; ++i) {
            const auto* r = input.historyAt(i);
            if (r && r->type == MorseElement::DIT) {
                sum += r->durationMs;
                n++;
            }
        }
        if (n >= 2) {
            float avg = sum / n;
            float var = 0;
            for (uint8_t i = 0; i < count && i < 5; ++i) {
                const auto* r = input.historyAt(i);
                if (r && r->type == MorseElement::DIT) {
                    float d = r->durationMs - avg;
                    var += d * d;
                }
            }
            var /= n;
            if (var > (ditMs * 0.3f) * (ditMs * 0.3f)) {
                setFeedback("Ritmo inconsistente. Pratique metronomo.");
            }
        }
    }

    setStatus("ANALISE");
}

const char* TrainerManager::modeName(AppMode m) {
    switch (m) {
        case AppMode::HOME:             return "INICIO";
        case AppMode::MENU_TRANSMIT:    return "TRANSMITIR";
        case AppMode::MENU_LESSONS:     return "LICOES";
        case AppMode::MONITOR:          return "MONITOR";
        case AppMode::TRAIN_LETTERS:    return "LETRAS";
        case AppMode::TRAIN_NUMBERS:    return "NUMEROS";
        case AppMode::TRAIN_WORDS:      return "PALAVRAS";
        case AppMode::TRAIN_CALLSIGN:   return "CALLSIGN";
        case AppMode::TRAIN_FREE:       return "LIVRE";
        case AppMode::PROFESSOR:        return "PROFESSOR";
        case AppMode::SETTINGS:         return "CONFIG";
        case AppMode::STATISTICS:       return "STATS";
        default:                        return "?";
    }
}

void TrainerManager::setWpm(uint8_t wpm) {
    _decoder.setWpm(wpm);
}

MorseDecoder& TrainerManager::decoder() {
    return _decoder;
}

const MorseDecoder& TrainerManager::decoder() const {
    return _decoder;
}

void TrainerManager::resetTrainingRound() {
    _score = 0;
    _streak = 0;
    _attempts = 0;
    _wordCharIndex = 0;
    _evaluated = false;
    _inputMorseLen = 0;
    _roundStartMs = millis();
}

void TrainerManager::pickRandomLetter() {
    _targetChar = 'A' + (esp_random() % 26);
    MorseDecoder::charToMorse(_targetChar, _targetMorse, sizeof(_targetMorse));
    requestTargetPlayback(_targetChar);
}

void TrainerManager::pickRandomNumber() {
    _targetChar = '0' + (esp_random() % 10);
    MorseDecoder::charToMorse(_targetChar, _targetMorse, sizeof(_targetMorse));
    requestTargetPlayback(_targetChar);
}

void TrainerManager::pickRandomWord() {
    _wordIndex = esp_random() % WORD_COUNT;
    strncpy(_targetWord, WORD_BANK[_wordIndex], sizeof(_targetWord) - 1);
    _targetWord[sizeof(_targetWord) - 1] = '\0';
    _wordCharIndex = 0;
}

void TrainerManager::generateCallsign() {
    static const char* prefixes[] = {"PY2", "PU2", "PP5", "PR7", "PS8"};
    uint8_t p = esp_random() % 5;
    char suffix[4];
    suffix[0] = 'A' + (esp_random() % 26);
    suffix[1] = 'A' + (esp_random() % 26);
    suffix[2] = 'A' + (esp_random() % 26);
    suffix[3] = '\0';
    snprintf(_targetCallsign, sizeof(_targetCallsign), "%s%s", prefixes[p], suffix);
    _wordCharIndex = 0;
}

void TrainerManager::evaluateCharacter(char decoded) {
    _attempts++;
    bool ok = (toupper(decoded) == _targetChar);

    if (_stats) {
        _stats->onCharacterSent(ok);
        if (ok) _stats->onStreakUpdate(_streak + 1);
    }

    if (ok) {
        _score++;
        _streak++;
        if (_stats) _stats->onStreakUpdate(_streak);
        setFeedback("OK CORRETO");
        setStatus("OK");
    } else {
        _streak = 0;
        char buf[64];
        snprintf(buf, sizeof(buf), "X INCORRETO | %c (%s)", _targetChar, _targetMorse);
        setFeedback(buf);
        setStatus("ERRO");
    }

    if (_mode == AppMode::TRAIN_LETTERS) pickRandomLetter();
    else pickRandomNumber();

    setStatus("TRANSMITA");
    setFeedback("Ouça e repita no paddle");
}

void TrainerManager::requestTargetPlayback(char c) {
    _playTargetChar = c;
    _playTargetPending = true;
}

void TrainerManager::replayTarget() {
    if (_mode == AppMode::TRAIN_LETTERS || _mode == AppMode::TRAIN_NUMBERS) {
        requestTargetPlayback(_targetChar);
    }
}

bool TrainerManager::pendingMorsePlayback() const {
    return _playTargetPending;
}

char TrainerManager::playbackChar() const {
    return _playTargetChar;
}

void TrainerManager::clearPlaybackPending() {
    _playTargetPending = false;
}

void TrainerManager::setFeedback(const char* msg) {
    strncpy(_feedback, msg ? msg : "", sizeof(_feedback) - 1);
    _feedback[sizeof(_feedback) - 1] = '\0';
}

void TrainerManager::setStatus(const char* msg) {
    strncpy(_status, msg ? msg : "", sizeof(_status) - 1);
    _status[sizeof(_status) - 1] = '\0';
}

char TrainerManager::randomCharFromPool(const char* pool) {
    size_t len = strlen(pool);
    if (len == 0) return 'E';
    return pool[esp_random() % len];
}
