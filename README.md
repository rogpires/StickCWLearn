# StickCWLearn — Intelligent Morse Code Teacher

[English](#english) | [Português](#português)

---

<a id="english"></a>

## English

Professional firmware for the **M5StickS3** that turns the device into a portable Morse code (CW) trainer for radio amateurs.

**GPIO-only input** — Paddle or Straight Key. No microphone, no DSP, no audio decoding.

---

### Requirements

| Item | Version |
|------|---------|
| Board | M5StickS3 (ESP32-S3) |
| IDE | Arduino IDE 2.x |
| Core | esp32 by Espressif (≥ 3.0) |
| Library | [M5Unified](https://github.com/m5stack/M5Unified) |
| Library | Preferences (included with ESP32 core) |

---

### Arduino IDE Setup

#### 1. Install the ESP32 core

1. **File → Preferences → Additional boards manager URLs:**
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. **Tools → Board → Boards Manager** → install **esp32**.
3. Select:
   - **Board:** ESP32S3 Dev Module (or M5Stack StickS3 if available)
   - **USB CDC On Boot:** Enabled
   - **PSRAM:** OPI PSRAM
   - **Partition Scheme:** Default 8MB

#### 2. Install M5Unified

**Sketch → Include Library → Manage Libraries** → search **M5Unified** → Install.

#### 3. Open and upload the sketch

1. Open the `StickCWLearn` folder (`StickCWLearn.ino` is the main file).
2. Connect the M5StickS3 via USB-C.
3. Select the correct serial port.
4. Click **Verify/Compile**, then **Upload**.

---

### Paddle Wiring

#### GPIO pins (configurable in `Config.h`)

| Function | ESP32-S3 GPIO | Hat2-Bus pin |
|----------|---------------|--------------|
| **DIT** | GPIO 5 | Pin 2 |
| **DAH** | GPIO 7 | Pin 8 |
| **GND** | GND | Pin 1 |

```cpp
#define GPIO_DIT  5
#define GPIO_DAH  7
```

#### Iambic paddle diagram

```
        M5StickS3 (Hat2-Bus)
        ┌─────────────────┐
  GND ──┤ 1  GND          │
        │ 2  G5  ─────────┼── Paddle DIT
        │ 8  G7  ─────────┼── Paddle DAH
        └─────────────────┘

Paddle (operator side):
  • DIT terminal  → GPIO 5
  • DAH terminal  → GPIO 7
  • Common (ground) → GND
```

Pins use internal **INPUT_PULLUP**: contacts **close to GND** when pressed (standard negative-common paddle wiring).

#### Straight key

```
  Fixed terminal  → GND
  Moving terminal → GPIO 5 (DIT)
```

In **Settings**, select **Straight Key**. GPIO DAH can remain disconnected.

#### Polarity inversion

If your paddle uses inverted logic (contact goes HIGH), enable **Invert contacts** in the settings menu.

---

### Reserved GPIOs (do not use for paddle)

| GPIO | Internal function |
|------|-------------------|
| 11, 12 | **BtnA** and **BtnB** (programmable buttons) |
| 39–45, 21, 38 | LCD display |
| 47, 48 | I2C (IMU, M5PM1) |
| 14–18 | ES8311 audio (sidetone) |
| 42, 46 | Infrared |

---

### Operating modes

| Mode | Description |
|------|-------------|
| **Monitor** | Shows DIT/DAH, sequence, and decoded text in real time |
| **Letter training** | Displays A–Z, evaluates your transmission |
| **Number training** | Trains 0–9 |
| **Word training** | Built-in bank: RADIO, MORSE, CQ, QSO, etc. |
| **Callsign training** | Generates PY/PU/PP/PR/PS style callsigns |
| **Free mode** | Free transmission with a 1024-character buffer |
| **CW Professor** | Analyzes rhythm, dot/dash length, and spacing |
| **Settings** | WPM, sidetone, paddle type, brightness, volume |
| **Statistics** | Sessions, hours, accuracy, best streak |

---

### Controls (built-in buttons)

> **M5StickS3:** use **BtnA** (side) and **BtnB** (front). The **power button** resets the device — do not use it for navigation.

#### General rule

| Context | BtnA | BtnB |
|---------|------|------|
| **Menus** (HOME, submenus) | Next | **ENTER** |
| **Activities** (training, professor, monitor) | Extra action* | **HOME** |
| **Settings** | Change value | Next (last item: **EXIT**) |
| **Statistics** | — | **HOME** |

\* Extra actions: Monitor = WPM+ | Letters/Numbers = **LISTEN** again | Free = clear text

#### Starting a lesson

1. **HOME** → BtnA until **LESSONS** → **BtnB**
2. BtnA until **LETTERS** → **BtnB**
3. Listen on the speaker → transmit on the paddle
4. **BtnB** → return to HOME

#### CW Professor

- Transmit on the paddle — the app analyzes rhythm and timing
- **BtnB** → HOME (only button to exit)
- BtnA does nothing (use the paddle)

#### Volume

Default is **90%**. Adjust in **CONFIG → VOLUME** with BtnA.

---

### Default settings

| Parameter | Default |
|-----------|---------|
| WPM | 18 (range 5–40) |
| Sidetone | 700 Hz (300–1000 Hz) |
| Paddle | Iambic B |
| Brightness | 100% |
| Volume | 90% |

All settings are saved to **Flash** via `Preferences` and persist after power-off.

---

### Project architecture

```
StickCWLearn/
├── StickCWLearn.ino       # Main entry (setup/loop)
├── Config.h               # GPIOs and constants
├── CWInput.h / .cpp       # Paddle, iambic keyer, debounce
├── MorseDecoder.h / .cpp  # Morse table and decoder
├── CWGenerator.h / .cpp   # Sidetone (M5.Speaker)
├── TrainerManager.h / .cpp # Training modes and Professor
├── DisplayManager.h / .cpp # UI
├── SettingsManager.h / .cpp
├── StatisticsManager.h / .cpp
└── README.md
```

---

### Sidetone

CW tone is generated through the **internal speaker** via `M5.Speaker` (M5StickS3 ES8311/I2S codec). There is no dedicated PWM buzzer GPIO; M5Unified generates a sine wave at the configured frequency.

Letter and number training modes automatically play the target character on the speaker before you repeat it on the paddle.

---

### Morse timing

Based on the **PARIS** standard (50 units per word):

- DIT = 1 unit = `1200 / WPM` ms
- DAH = 3 units
- Inter-element gap = 1 unit
- Inter-letter gap = 3 units
- Inter-word gap = 7 units

---

### License

Open source project for the amateur radio community.

73! Rogerio - PY2EQ 📻

---

<a id="português"></a>

## Português

Firmware profissional para **M5StickS3** que transforma o dispositivo em um treinador portátil de Código Morse (CW) para radioamadores.

**Entrada exclusiva por GPIO** — Paddle ou Straight Key. Sem microfone, sem DSP, sem decodificação de áudio.

---

### Requisitos

| Item | Versão |
|------|--------|
| Placa | M5StickS3 (ESP32-S3) |
| IDE | Arduino IDE 2.x |
| Core | esp32 by Espressif (≥ 3.0) |
| Biblioteca | [M5Unified](https://github.com/m5stack/M5Unified) |
| Biblioteca | Preferences (incluída no core ESP32) |

---

### Instalação na Arduino IDE

#### 1. Configurar o core ESP32

1. **Arquivo → Preferências → URLs adicionais:**
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. **Ferramentas → Placa → Gerenciador de placas** → instalar **esp32**.
3. Selecionar:
   - **Placa:** ESP32S3 Dev Module (ou M5Stack StickS3 se disponível)
   - **USB CDC On Boot:** Enabled
   - **PSRAM:** OPI PSRAM
   - **Partition Scheme:** Default 8MB

#### 2. Instalar M5Unified

**Sketch → Incluir Biblioteca → Gerenciar Bibliotecas** → buscar **M5Unified** → Instalar.

#### 3. Abrir e compilar o sketch

1. Abra a pasta `StickCWLearn` (o arquivo principal é `StickCWLearn.ino`).
2. Conecte o M5StickS3 via USB-C.
3. Selecione a porta serial correta.
4. Clique em **Verificar/Compilar** e depois **Carregar**.

---

### Ligação elétrica do Paddle

#### GPIOs utilizados (configuráveis em `Config.h`)

| Função | GPIO ESP32-S3 | Pino Hat2-Bus |
|--------|---------------|---------------|
| **DIT** | GPIO 5 | Pino 2 |
| **DAH** | GPIO 7 | Pino 8 |
| **GND** | GND | Pino 1 |

```cpp
#define GPIO_DIT  5
#define GPIO_DAH  7
```

#### Diagrama de ligação — Paddle Iâmbico

```
        M5StickS3 (Hat2-Bus)
        ┌─────────────────┐
  GND ──┤ 1  GND          │
        │ 2  G5  ─────────┼── DIT do Paddle
        │ 8  G7  ─────────┼── DAH do Paddle
        └─────────────────┘

Paddle (lado do operador):
  • Terminal DIT  → GPIO 5
  • Terminal DAH  → GPIO 7
  • Comum (ground) → GND
```

Os pinos usam **INPUT_PULLUP** interno: o contato **fecha para GND** quando pressionado (padrão de paddles com fio comum no negativo).

#### Straight Key (chave manual)

```
  Terminal fixo  → GND
  Terminal móvel → GPIO 5 (DIT)
```

Nas **Configurações**, selecione **Straight Key**. O GPIO DAH pode ficar desconectado.

#### Inversão de polaridade

Se o seu paddle usa lógica invertida (contato em HIGH), ative **Inverter contatos** no menu de configurações.

---

### GPIOs reservados (não usar para paddle)

| GPIO | Função interna |
|------|----------------|
| 11, 12 | Botões **BtnA** e **BtnB** (programáveis) |
| 39–45, 21, 38 | Display LCD |
| 47, 48 | I2C (IMU, M5PM1) |
| 14–18 | Áudio ES8311 (sidetone) |
| 42, 46 | Infravermelho |

---

### Modos de operação

| Modo | Descrição |
|------|-----------|
| **Monitor** | Exibe DIT/DAH, sequência e texto decodificado em tempo real |
| **Treino Letras** | Exibe A–Z, avalia transmissão |
| **Treino Números** | Treina 0–9 |
| **Treino Palavras** | Banco: RADIO, MORSE, CQ, QSO, etc. |
| **Treino Callsign** | Gera indicativos PY/PU/PP/PR/PS |
| **Modo Livre** | Transmissão livre com buffer de 1024 caracteres |
| **Professor CW** | Analisa ritmo, duração de pontos/traços e espaçamento |
| **Configurações** | WPM, sidetone, paddle, brilho, volume |
| **Estatísticas** | Sessões, horas, precisão, melhor sequência |

---

### Controles (botões integrados)

> **M5StickS3:** use **BtnA** (lateral) e **BtnB** (frontal). O botão de **energia** reinicia o aparelho.

#### Regra geral

| Contexto | BtnA | BtnB |
|----------|------|------|
| **Menus** (INICIO, submenus) | Próximo | **ENTRAR** |
| **Atividades** (treino, professor, monitor) | Ação extra* | **INICIO** |
| **Config** | Alterar valor | Próximo (último: **SAIR**) |
| **Estatísticas** | — | **INICIO** |

\* Ações extras: Monitor = WPM+ | Letras/Números = **OUVIR** de novo | Livre = limpar texto

#### Fluxo para começar uma lição

1. **INICIO** → BtnA até **LICOES** → **BtnB**
2. BtnA até **LETRAS** → **BtnB**
3. Ouça no speaker → transmita no paddle
4. **BtnB** → volta ao INICIO

#### Professor CW

- Transmita no paddle — o app analisa ritmo e duração
- **BtnB** → INICIO (único botão para sair)
- BtnA não faz nada (use o paddle)

#### Volume

Padrão em **90%**. Ajuste em **CONFIG → VOLUME** com BtnA.

---

### Configurações padrão

| Parâmetro | Padrão |
|-----------|--------|
| WPM | 18 (faixa 5–40) |
| Sidetone | 700 Hz (300–1000 Hz) |
| Paddle | Iambic B |
| Brilho | 100% |
| Volume | 90% |

Todas as configurações são salvas em **Flash** via `Preferences` e persistem após desligamento.

---

### Arquitetura do projeto

```
StickCWLearn/
├── StickCWLearn.ino      # Entrada principal (setup/loop)
├── Config.h              # GPIOs e constantes
├── CWInput.h / .cpp      # Paddle, iâmbico, debounce
├── MorseDecoder.h / .cpp # Tabela Morse e decodificação
├── CWGenerator.h / .cpp  # Sidetone (M5.Speaker)
├── TrainerManager.h / .cpp # Modos de treino e Professor
├── DisplayManager.h / .cpp # Interface gráfica
├── SettingsManager.h / .cpp
├── StatisticsManager.h / .cpp
└── README.md
```

---

### Sidetone

O tom CW é gerado pelo **alto-falante interno** via API `M5.Speaker` (codec ES8311/I2S do M5StickS3). O hardware não possui buzzer PWM dedicado; o M5Unified gera a forma de onda senoidal na frequência configurada.

Nos modos de treino de letras e números, o caractere alvo é reproduzido automaticamente no speaker antes de você repetir no paddle.

---

### Temporização Morse

Baseada no padrão **PARIS** (50 unidades por palavra):

- DIT = 1 unidade = `1200 / WPM` ms
- DAH = 3 unidades
- Espaço entre elementos = 1 unidade
- Espaço entre letras = 3 unidades
- Espaço entre palavras = 7 unidades

---

### Licença

Projeto open source para a comunidade de radioamadores.

73! Rogerio - PY2EQ 📻
