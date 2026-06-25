#pragma once

// =============================================================================
// Config.h — Configuração de hardware e constantes do CW Teacher
// M5StickS3 — Professor Inteligente de Telegrafia Morse
// =============================================================================

#include <Arduino.h>

// -----------------------------------------------------------------------------
// GPIOs do Paddle / Straight Key (Hat2-Bus — facilmente alteráveis)
// -----------------------------------------------------------------------------
// Pinout Hat2-Bus recomendado:
//   DIT  → GPIO 5  (Hat2 pin 2)
//   DAH  → GPIO 7  (Hat2 pin 8)
//   GND  → GND     (Hat2 pin 1)
//
// Para Straight Key: conecte um terminal ao GND e o outro ao GPIO_DIT.
// Deixe GPIO_DAH desconectado e selecione modo STRAIGHT_KEY nas configurações.
// -----------------------------------------------------------------------------
#ifndef GPIO_DIT
#define GPIO_DIT  5
#endif

#ifndef GPIO_DAH
#define GPIO_DAH  7
#endif

// -----------------------------------------------------------------------------
// Limites operacionais
// -----------------------------------------------------------------------------
#define WPM_MIN              5
#define WPM_MAX              40
#define WPM_DEFAULT          18

#define SIDETONE_FREQ_MIN    300
#define SIDETONE_FREQ_MAX    1000
#define SIDETONE_FREQ_DEFAULT 700

#define DEBOUNCE_MS          15
#define DISPLAY_REFRESH_MS   80

// Áudio CW — ES8311 + alto-falante I2S (M5.Speaker.tone, não buzzer piezo)
#define CW_AUDIO_CHANNEL     0
#define CW_ATTACK_MS         10
#define CW_RELEASE_MS        15
#define CW_SILENCE_PAD_MS    4
#define CW_ATTACK_STEPS      5
#define CW_RELEASE_STEPS     5
#define SESSION_MAX_CHARS    1024
#define FREE_SCROLL_LINES    6

// Tolerâncias do modo Professor (fração do tempo de um DIT)
#define PROF_DIT_MIN_RATIO   0.45f
#define PROF_DIT_MAX_RATIO   1.60f
#define PROF_DAH_MIN_RATIO   2.40f
#define PROF_DAH_MAX_RATIO   3.80f
#define PROF_LETTER_GAP_MIN  2.50f
#define PROF_LETTER_GAP_MAX  5.00f
#define PROF_WORD_GAP_MIN    5.50f

// -----------------------------------------------------------------------------
// Enums compartilhados
// -----------------------------------------------------------------------------
enum class PaddleMode : uint8_t {
    IAMBIC_A = 0,
    IAMBIC_B = 1,
    STRAIGHT_KEY = 2
};

enum class AppMode : uint8_t {
    HOME = 0,           // Tela inicial — hub principal
    MENU_TRANSMIT,      // Submenu: Monitor / Modo Livre
    MENU_LESSONS,       // Submenu: Letras / Numeros / Palavras / Callsign
    MONITOR,
    TRAIN_LETTERS,
    TRAIN_NUMBERS,
    TRAIN_WORDS,
    TRAIN_CALLSIGN,
    TRAIN_FREE,
    PROFESSOR,
    SETTINGS,
    STATISTICS
};

enum class MorseElement : uint8_t {
    NONE = 0,
    DIT,
    DAH
};

enum class KeyEventType : uint8_t {
    TONE_ON,
    TONE_OFF,
    ELEMENT_COMPLETE
};

struct KeyEvent {
    KeyEventType type;
    MorseElement element;
    uint32_t durationMs;
};

// Temporização Morse PARIS: 50 unidades por palavra "PARIS"
inline uint32_t ditDurationMs(uint8_t wpm) {
    if (wpm < WPM_MIN) wpm = WPM_MIN;
    if (wpm > WPM_MAX) wpm = WPM_MAX;
    return 1200UL / wpm;
}

inline uint32_t dahDurationMs(uint8_t wpm) {
    return ditDurationMs(wpm) * 3;
}

inline uint32_t elementGapMs(uint8_t wpm) {
    return ditDurationMs(wpm);
}

inline uint32_t letterGapMs(uint8_t wpm) {
    return ditDurationMs(wpm) * 3;
}

inline uint32_t wordGapMs(uint8_t wpm) {
    return ditDurationMs(wpm) * 7;
}
