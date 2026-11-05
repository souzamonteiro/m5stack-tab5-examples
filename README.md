# M5Stack Tab5 Examples

Exemplos de código e testes de periféricos para o **M5Stack Tab5 (ESP32-P4 + companion ESP32-C6)** com display ST7121 MIPI-DSI de 5" (1280x720) e teclado físico STM32 A164.

Todos os exemplos utilizam arquitetura **cooperativa e não-bloqueante** (sem depender de FreeRTOS), servindo como especificação e modelo de referência para o desenvolvimento de sistemas operacionais bare-metal (como o [MaiaRTOS / MaiaOS](https://github.com/roberto-l-monteiro/maia-rtos)).

---

## Requisitos de Hardware e Software

1. **Placa:** M5Stack Tab5 (ESP32-P4 v1.3 com PSRAM).
2. **Pacote de Placas:** M5Stack Board Package (versão 3.2.2 ou superior) no Arduino IDE / arduino-cli.
3. **Bibliotecas Arduino:**
   - `M5Unified` (v0.2.29+)
   - `M5GFX` (v0.2.36+)
   - `SD_MMC`, `FS`, `WiFi` (inclusos no core ESP32-P4).

---

## Exemplos Disponíveis

| Exemplo | Subsistemas Testados | Descrição |
|---|---|---|
| **`Blink`** | Display + Bateria | Monitor de sistema e telemetria de tensão/carga da bateria. |
| **`Echo`** | Display + Teclado Físico | Leitura do teclado físico I2C (`0x6D`) em modo caractere e eco na tela. |
| **`Touch`** | Touchscreen capacitivo | Leitura e rastreamento de coordenadas `(X, Y)` em tempo real. |
| **`Scheduler`** | Multitarefa Cooperativa | Scheduler dinâmico baseado em vetores de tarefas por temporização (`millis`). |
| **`Tasks`** | Teclado + Wi-Fi + LED | Execução concorrente de digitação no teclado, varredura Wi-Fi e indicador visual. |
| **`WiFi`** | ESP32-C6 Co-Processor | Inicialização em modo STA e escaneamento das redes sem fio locais. |
| **`Movement`** | IMU BMI270 + Gráficos | Leitura de acelerômetro/giroscópio 6-eixos e simulação física de uma bolinha na tela. |
| **`MicroSD`** | Barramento SDIO 4-bits | Montagem do cartão, gravação de arquivo e leitura de volta no LCD. |
| **`FullTest`** | Todos os Periféricos | Painel completo integrando Teclado, IMU, Touch, Bateria, Wi-Fi, MicroSD e CPU Load %. |

---

## Como Compilar e Gravar via Makefile

O repositório inclui um `Makefile` que detecta automaticamente o `arduino-cli` e a porta serial conectada (`/dev/cu.usbmodem*` no macOS ou `/dev/ttyACM*` / `/dev/ttyUSB*` no Linux).

### 1. Ver comandos e status
```bash
make help
```

### 2. Compilar um exemplo
```bash
make FullTest
# ou
make compile SKETCH=FullTest
```

### 3. Gravar na placa (compila e envia)
```bash
make flash-FullTest
# ou
make flash SKETCH=FullTest
```

### 4. Compilar todos os exemplos de uma vez
```bash
make all
```

### 5. Monitor Serial
```bash
make monitor
```

---

## Protocolos de Hardware Importantes

### Teclado Físico (STM32 A164 @ I2C 0x6D)
- **Barramento:** SDA no `GPIO 0`, SCL no `GPIO 1` (400 kHz).
- **Modo Caractere:** Gravar `0x02` no registrador `0x10`.
- **Habilitar Interrupções:** Gravar `0x07` no registrador `0x00`.
- **Leitura da Fila:**
  1. Ler tamanho de eventos no registrador `0x40`.
  2. Se `len > 0`, ler `len + 1` bytes a partir de `0x50` (byte 0 = modificadores, bytes 1..len = caracteres).
  3. Limpar interrupção gravando `0x00` no registrador `0x01`.

### Cartão MicroSD (SDIO 4-Bits)
- `SD_D0`: `GPIO 39`
- `SD_D1`: `GPIO 40`
- `SD_D2`: `GPIO 41`
- `SD_D3`: `GPIO 42`
- `SD_CLK`: `GPIO 43`
- `SD_CMD`: `GPIO 44`
