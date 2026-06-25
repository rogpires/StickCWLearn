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
    _wordCharIndex = 0;
    _freeLen = 0;
    _freeText[0] = '\0';
    _status[0] = _feedback[0] = '\0';
    _playPending = PlaybackType::None;
    _decoder.reset();
    _prefs.begin("cwsess", false);
}

void TrainerManager::setMode(AppMode mode) {
    _mode = mode;
    _score = _streak = _attempts = 0;
    _wordCharIndex = 0;
    _decoder.reset();
    _playPending = PlaybackType::None;

    switch (mode) {
        case AppMode::TRAIN_LETTERS:
            pickRandomLetter();
            break;
        case AppMode::TRAIN_NUMBERS:
            pickRandomNumber();
            break;
        case AppMode::TRAIN_WORDS:
            pickRandomWord();
            break;
        case AppMode::TRAIN_CALLSIGN:
            generateCallsign();
            break;
        case AppMode::TRAIN_FREE:
            loadLastFreeSession();
            setStatus("LIVRE");
            setFeedback("");
            break;
        case AppMode::MONITOR:
            setStatus("PRONTO");
            setFeedback("Transmita no paddle");
            break;
        default:
            break;
    }
}

AppMode TrainerManager::mode() const { return _mode; }

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
            break;

        case AppMode::TRAIN_LETTERS:
        case AppMode::TRAIN_NUMBERS:
            evaluateCharacter(c);
            break;

        case AppMode::TRAIN_WORDS:
        case AppMode::TRAIN_CALLSIGN: {
            const char* target = currentTarget();
            if (_wordCharIndex >= strlen(target)) break;

            char expected = target[_wordCharIndex];
            bool ok = (c == expected);
            _attempts++;
            if (_stats) _stats->onCharacterSent(ok);

            if (ok) {
                _score++;
                _streak++;
                if (_stats) _stats->onStreakUpdate(_streak);
                setFeedback("OK!");
            } else {
                _streak = 0;
                char buf[32];
                snprintf(buf, sizeof(buf), "Erro: %c", expected);
                setFeedback(buf);
            }

            _wordCharIndex++;
            if (_wordCharIndex >= strlen(target)) {
                setStatus("FIM");
                setFeedback("Rodada completa!");
                if (_mode == AppMode::TRAIN_WORDS) pickRandomWord();
                else generateCallsign();
            }
            break;
        }

        case AppMode::TRAIN_FREE:
            if (_freeLen < SESSION_MAX_CHARS) {
                _freeText[_freeLen++] = c;
                _freeText[_freeLen] = '\0';
            }
            if (_decoder.wordJustEnded() && _freeLen < SESSION_MAX_CHARS - 1) {
                _freeText[_freeLen++] = ' ';
                _freeText[_freeLen] = '\0';
                _decoder.clearWordFlag();
            }
            break;

        default:
            break;
    }
}

void TrainerManager::onPlaybackFinished() {
    if (_mode >= AppMode::TRAIN_LETTERS && _mode <= AppMode::TRAIN_CALLSIGN) {
        setStatus("TRANSMITA");
        setFeedback("Repita no paddle");
    }
}

const char* TrainerManager::statusText() const { return _status; }
const char* TrainerManager::feedbackText() const { return _feedback; }
char TrainerManager::targetChar() const { return _targetChar; }
const char* TrainerManager::targetWord() const { return _targetWord; }
const char* TrainerManager::targetCallsign() const { return _targetCallsign; }

const char* TrainerManager::currentTarget() const {
    switch (_mode) {
        case AppMode::TRAIN_WORDS: return _targetWord;
        case AppMode::TRAIN_CALLSIGN: return _targetCallsign;
        default: return "";
    }
}

uint16_t TrainerManager::score() const { return _score; }
uint16_t TrainerManager::streak() const { return _streak; }
uint8_t TrainerManager::targetProgress() const { return _wordCharIndex; }

void TrainerManager::clearFreeText() {
    _freeLen = 0;
    _freeText[0] = '\0';
    _decoder.reset();
}

void TrainerManager::saveFreeSession() {
    _prefs.putString("text", _freeText);
}

void TrainerManager::loadLastFreeSession() {
    String saved = _prefs.getString("text", "");
    if (saved.length() > 0) {
        strncpy(_freeText, saved.c_str(), SESSION_MAX_CHARS);
        _freeText[SESSION_MAX_CHARS] = '\0';
        _freeLen = strlen(_freeText);
    }
}

const char* TrainerManager::freeText() const {
    return (_mode == AppMode::TRAIN_FREE) ? _freeText : _decoder.decodedText();
}

const char* TrainerManager::modeName(AppMode m) {
    switch (m) {
        case AppMode::HOME:           return "MENU";
        case AppMode::MONITOR:        return "MONITOR";
        case AppMode::TRAIN_LETTERS:  return "LETRAS";
        case AppMode::TRAIN_NUMBERS:  return "NUMEROS";
        case AppMode::TRAIN_WORDS:    return "PALAVRAS";
        case AppMode::TRAIN_CALLSIGN: return "CALLSIGN";
        case AppMode::TRAIN_FREE:     return "LIVRE";
        case AppMode::SETTINGS:       return "CONFIG";
        default:                      return "CW";
    }
}

void TrainerManager::setWpm(uint8_t wpm) { _decoder.setWpm(wpm); }
MorseDecoder& TrainerManager::decoder() { return _decoder; }
const MorseDecoder& TrainerManager::decoder() const { return _decoder; }

void TrainerManager::pickRandomLetter() {
    _targetChar = 'A' + (esp_random() % 26);
    MorseDecoder::charToMorse(_targetChar, _targetMorse, sizeof(_targetMorse));
    _wordCharIndex = 0;
    setStatus("OUVINDO");
    setFeedback("");
    requestPlayChar(_targetChar);
}

void TrainerManager::pickRandomNumber() {
    _targetChar = '0' + (esp_random() % 10);
    MorseDecoder::charToMorse(_targetChar, _targetMorse, sizeof(_targetMorse));
    _wordCharIndex = 0;
    setStatus("OUVINDO");
    setFeedback("");
    requestPlayChar(_targetChar);
}

void TrainerManager::pickRandomWord() {
    _wordIndex = esp_random() % WORD_COUNT;
    strncpy(_targetWord, WORD_BANK[_wordIndex], sizeof(_targetWord) - 1);
    _targetWord[sizeof(_targetWord) - 1] = '\0';
    _wordCharIndex = 0;
    setStatus("OUVINDO");
    setFeedback("");
    requestPlayText(_targetWord);
}

void TrainerManager::generateCallsign() {
    static const char* prefixes[] = {"PY2", "PU2", "PP5", "PR7", "PS8"};
    uint8_t p = esp_random() % 5;
    char suffix[4] = {
        char('A' + (esp_random() % 26)),
        char('A' + (esp_random() % 26)),
        char('A' + (esp_random() % 26)),
        '\0'
    };
    snprintf(_targetCallsign, sizeof(_targetCallsign), "%s%s", prefixes[p], suffix);
    _wordCharIndex = 0;
    setStatus("OUVINDO");
    setFeedback("");
    requestPlayText(_targetCallsign);
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
        setFeedback("OK!");
    } else {
        _streak = 0;
        char buf[32];
        snprintf(buf, sizeof(buf), "Era: %c (%s)", _targetChar, _targetMorse);
        setFeedback(buf);
    }

    if (_mode == AppMode::TRAIN_LETTERS) pickRandomLetter();
    else pickRandomNumber();
}

void TrainerManager::requestPlayChar(char c) {
    _playChar = c;
    _playPending = PlaybackType::Char;
}

void TrainerManager::requestPlayText(const char* text) {
    strncpy(_playText, text ? text : "", sizeof(_playText) - 1);
    _playText[sizeof(_playText) - 1] = '\0';
    _playPending = PlaybackType::Text;
}

void TrainerManager::schedulePlaybackForCurrentTarget() {
    switch (_mode) {
        case AppMode::TRAIN_LETTERS:
        case AppMode::TRAIN_NUMBERS:
            requestPlayChar(_targetChar);
            break;
        case AppMode::TRAIN_WORDS:
            requestPlayText(_targetWord);
            break;
        case AppMode::TRAIN_CALLSIGN:
            requestPlayText(_targetCallsign);
            break;
        default:
            break;
    }
    setStatus("OUVINDO");
    setFeedback("");
}

void TrainerManager::replayTarget() {
    schedulePlaybackForCurrentTarget();
}

bool TrainerManager::pendingPlayback() const {
    return _playPending != PlaybackType::None;
}

bool TrainerManager::playbackIsText() const {
    return _playPending == PlaybackType::Text;
}

char TrainerManager::playbackChar() const { return _playChar; }
const char* TrainerManager::playbackText() const { return _playText; }

void TrainerManager::clearPlaybackPending() {
    _playPending = PlaybackType::None;
}

void TrainerManager::setFeedback(const char* msg) {
    strncpy(_feedback, msg ? msg : "", sizeof(_feedback) - 1);
    _feedback[sizeof(_feedback) - 1] = '\0';
}

void TrainerManager::setStatus(const char* msg) {
    strncpy(_status, msg ? msg : "", sizeof(_status) - 1);
    _status[sizeof(_status) - 1] = '\0';
}
