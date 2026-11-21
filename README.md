# M5Stack Tab5 Examples

Exemplos de código e testes de periféricos para o **M5Stack Tab5 (ESP32-P4 + companion ESP32-C6)** com display ST7121 MIPI-DSI de 5" (1280x720) e teclado físico STM32 A164.

Os exemplos usam o core Arduino/ESP-IDF e bibliotecas M5Stack como referência
para o desenvolvimento de sistemas bare-metal (como o
[MaiaRTOS / MaiaOS](https://github.com/roberto-l-monteiro/maia-rtos)).
A organização cooperativa do código de aplicação não elimina o runtime
FreeRTOS do core; essas dependências precisam ser removidas nos ports nativos.

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
| **`MicroSDReference`** | SDMMC bruto 4-bits | Referência somente de leitura dos setores selecionados e varredura limitada da FAT32 para localizar um cluster livre; sem montar filesystem. |
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

### Referência externa de leitura SD

`MicroSDReference` usa ESP-IDF v5.5.4 fornecido pelo core Arduino M5Stack
3.3.10, em slot 0/4 bits/20 MHz, com controle de alimentação LDO4. Esse firmware
auxiliar depende do runtime Arduino/ESP-IDF/FreeRTOS; não faz parte do MaiaRTOS
nativo e não demonstra independência de FreeRTOS.

Após o boot, envie `R` pela serial a 115200 baud em até 30 segundos. O programa
identifica o cartão e imprime os 512 bytes de cada LBA em registros
`REFERENCE_DATA`, seguidos por metadados MBR/BPB/FAT32 e o primeiro cluster livre
encontrado nos primeiros 4096 setores da FAT. `REFERENCE_FREE_DATA`
contém o setor original desse cluster para permitir backup/verificação.
O programa verifica os FATs espelhados (ou a FAT ativa indicada pelo BPB) e
emite `MAIA_SDMMC_REFERENCE_PASS`; falhas emitem `REFERENCE_FAT_FAIL`.
Não monta FatFs, abre arquivos, escreve setores ou formata o cartão.

Compile com `make MicroSDReference`. A gravação substitui temporariamente o
firmware na flash: preserve e verifique um backup antes de
`make flash-MicroSDReference`, e restaure/verifique esse backup depois da comparação.
Compare os mesmos LBAs e todos os bytes com o driver sob teste. O LBA candidato
é um cluster livre segundo a FAT, não uma partição reservada; mantenha o cartão
sem outras montagens enquanto o teste temporário for executado. Esta referência
usa outro driver, mas o mesmo controlador e cartão; não substitui testes de
remoção, falhas, alimentação ou outro controlador físico.

No teste de referência, os quatro setores coincidiram com as capturas nativas.
A flash de 16 MiB foi restaurada e comparada integralmente antes do reboot.
O reset RTS não retomou o firmware legado; o reset oficial esptool
`--after watchdog_reset chip_id` retomou o sistema, e o usuário confirmou
desktop e teclado. Reabra a porta após o reset USB e evite pulsos DTR/RTS
involuntários ao abrir a serial do firmware restaurado. O boot legado pode
sincronizar arquivos no SD; isso não é uma escrita feita pela referência bruta.

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
