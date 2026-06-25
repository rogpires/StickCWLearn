# StickCWLearn — Professor Inteligente de Telegrafia Morse

Firmware profissional para **M5StickS3** que transforma o dispositivo em um treinador portátil de Código Morse (CW) para radioamadores.

**Entrada exclusiva por GPIO** — Paddle ou Straight Key. Sem microfone, sem DSP, sem decodificação de áudio.

---

## Requisitos

| Item | Versão |
|------|--------|
| Placa | M5StickS3 (ESP32-S3) |
| IDE | Arduino IDE 2.x |
| Core | esp32 by Espressif (≥ 3.0) |
| Biblioteca | [M5Unified](https://github.com/m5stack/M5Unified) |
| Biblioteca | Preferences (incluída no core ESP32) |

---

## Instalação na Arduino IDE

### 1. Configurar o core ESP32

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

### 2. Instalar M5Unified

**Sketch → Incluir Biblioteca → Gerenciar Bibliotecas** → buscar **M5Unified** → Instalar.

### 3. Abrir e compilar o sketch

1. Abra a pasta `StickCWLearn` (o arquivo principal é `StickCWLearn.ino`).
2. Conecte o M5StickS3 via USB-C.
3. Selecione a porta serial correta.
4. Clique em **Verificar/Compilar** e depois **Carregar**.

---

## Ligação elétrica do Paddle

### GPIOs utilizados (configuráveis em `Config.h`)

| Função | GPIO ESP32-S3 | Pino Hat2-Bus |
|--------|---------------|---------------|
| **DIT** | GPIO 5 | Pino 2 |
| **DAH** | GPIO 7 | Pino 8 |
| **GND** | GND | Pino 1 |

```cpp
#define GPIO_DIT  5
#define GPIO_DAH  7
```

### Diagrama de ligação — Paddle Iâmbico

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

### Straight Key (chave manual)

```
  Terminal fixo  → GND
  Terminal móvel  → GPIO 5 (DIT)
```

Nas **Configurações**, selecione **Straight Key**. O GPIO DAH pode ficar desconectado.

### Inversão de polaridade

Se o seu paddle usa lógica invertida (contato em HIGH), ative **Inverter contatos** no menu de configurações ou altere `invertContacts` nas configurações salvas.

---

## GPIOs reservados (não usar para paddle)

| GPIO | Função interna |
|------|----------------|
| 11, 12 | Botões **BtnA** e **BtnB** (programáveis) |
| 39–45, 21, 38 | Display LCD |
| 47, 48 | I2C (IMU, M5PM1) |
| 14–18 | Áudio ES8311 (sidetone) |
| 42, 46 | Infravermelho |

---

## Modos de operação

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

## Controles (botões integrados)

> **M5StickS3:** use **BtnA** (lateral) e **BtnB** (frontal). O botão de **energia** reinicia o aparelho.

### Regra geral

| Contexto | BtnA | BtnB |
|----------|------|------|
| **Menus** (INICIO, submenus) | Próximo | **ENTRAR** |
| **Atividades** (treino, professor, monitor) | Ação extra* | **INICIO** |
| **Config** | Alterar valor | Próximo (último: **SAIR**) |
| **Estatísticas** | — | **INICIO** |

\* Ações extras: Monitor = WPM+ | Letras/Números = **OUVIR** de novo | Livre = limpar texto

### Fluxo para começar uma lição

1. **INICIO** → BtnA até **LICOES** → **BtnB**
2. BtnA até **LETRAS** → **BtnB**
3. Ouça no speaker → transmita no paddle
4. **BtnB** → volta ao INICIO

### Professor CW

- Transmita no paddle — o app analisa ritmo e duração
- **BtnB** → INICIO (único botão para sair)
- BtnA não faz nada (use o paddle)

### Volume

Padrão aumentado para **90%**. Ajuste em **CONFIG → VOLUME** com BtnA.

---

## Configurações padrão

| Parâmetro | Padrão |
|-----------|--------|
| WPM | 18 (faixa 5–40) |
| Sidetone | 700 Hz (300–1000 Hz) |
| Paddle | Iambic B |
| Brilho | 80% |
| Volume | 60% |

Todas as configurações são salvas em **Flash** via `Preferences` e persistem após desligamento.

---

## Arquitetura do projeto

```
StickCWLearn/
├── StickCWLearn.ino      # Entrada principal (setup/loop)
├── Config.h              # GPIOs e constantes
├── CWInput.h / .cpp      # Paddle, iâmbico, debounce
├── MorseDecoder.h / .cpp # Tabela Morse e decodificação
├── CWGenerator.h / .cpp  # Sidetone (M5.Speaker)
├── TrainerManager.h / .cpp # Modos de treino e Professor
├── DisplayManager.h / .cpp # Interface em Português
├── SettingsManager.h / .cpp
├── StatisticsManager.h / .cpp
└── README.md
```

---

## Sidetone

O tom CW é gerado pelo **alto-falante interno** via API `M5.Speaker` (codec ES8311/I2S do M5StickS3). O hardware não possui buzzer PWM dedicado; o M5Unified gera a forma de onda senoidal na frequência configurada.

---

## Temporização Morse

Baseada no padrão **PARIS** (50 unidades por palavra):

- DIT = 1 unidade = `1200 / WPM` ms  
- DAH = 3 unidades  
- Espaço entre elementos = 1 unidade  
- Espaço entre letras = 3 unidades  
- Espaço entre palavras = 7 unidades  

---

## Licença

Projeto open source para a comunidade de radioamadores.

73! Rogerio - PY2EQ 📻
