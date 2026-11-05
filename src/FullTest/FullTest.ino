#include <M5Unified.h>
#include <Wire.h>
#include <FS.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <vector>

// ====================================================================
// M5Stack Tab5 - Teste Geral Completo de Hardware (FullTest)
// Execução Cooperativa e Não-Bloqueante (Sem RTOS)
// ====================================================================

// --- HARDWARE CONFIG: TECLADO FÍSICO I2C (STM32 A164) ---
#define TAB5_KEYBOARD_ADDR 0x6D
const int8_t KEYBOARD_SDA = 0;
const int8_t KEYBOARD_SCL = 1;

// --- HARDWARE CONFIG: MICRO SD (SDIO 4-BITS) ---
#define SD_D0  GPIO_NUM_39
#define SD_D1  GPIO_NUM_40
#define SD_D2  GPIO_NUM_41
#define SD_D3  GPIO_NUM_42
#define SD_CLK GPIO_NUM_43
#define SD_CMD GPIO_NUM_44

// --- HARDWARE CONFIG: ÁUDIO ---
#define AUDIO_BUF_SIZE 512
int16_t bufferAudio[AUDIO_BUF_SIZE];
bool audioPassThroughAtivo = false;

// --- VARIÁVEIS DE ESTADO DO SISTEMA ---
String textoTeclado = "Tab5 pronto para digitar...";
int touchX = -1, touchY = -1;
float accelX = 0, accelY = 0, accelZ = 0;
float gyroX = 0, gyroY = 0, gyroZ = 0;
float tensaoBat = 0.0;
bool carregando = false;
bool sdMontado = false;
uint64_t sdTotalMB = 0;
int redesWiFiEncontradas = -1;
bool ledPisca = false;
bool precisaAtualizarTeclado = true;

// Mini horizonte artificial (posição anterior da bolinha)
int bolaNivelX = 945;
int bolaNivelY = 245;

// --- MEDIDOR DE CARGA DE CPU ---
unsigned long contagemCiclos = 0;
unsigned long ciclosOciosoMax = 0;
float cargaCPU = 0.0;

// --- ESTRUTURA DO SCHEDULER COOPERATIVO ---
struct Tarefa {
    void (*funcao)();
    unsigned long intervalo;
    unsigned long tempoAnterior;
    bool ativa;
};
std::vector<Tarefa> scheduler;

// --- PROTÓTIPOS DAS TAREFAS ---
void tarefaTeclado();
void tarefaTouch();
void tarefaSensoresIMU();
void tarefaAudio();
void tarefaAtualizaSensores();
void tarefaTelemetriaDisplay();
void tarefaCalculaCPULoad();
void tarefaPiscaIndicador();

// ====================================================================
// SETUP
// ====================================================================
void setup() {
    Serial.begin(115200);

    // 1. Inicializa o ecossistema M5Unified
    auto cfg = M5.config();
    M5.begin(cfg);

    // 2. Display em modo Paisagem (1280 x 720) para casar com o teclado físico
    M5.Display.setRotation(3);
    M5.Display.setBrightness(128);
    M5.Display.clear(BLACK);

    // Cabeçalho estático da aplicação
    M5.Display.setTextSize(2.5);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString("M5Stack Tab5 - Full System Test (Cooperativo)", 30, 20);
    M5.Display.drawFastHLine(30, 55, 1220, TFT_DARKGRAY);

    // Desenha as molduras estáticas dos 3 painéis principais
    // Painel 1: Teclado (Superior Esquerdo)
    M5.Display.drawRect(30, 70, 590, 280, TFT_DARKGRAY);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString("[TECLADO FISICO I2C]", 45, 82);
    M5.Display.setTextColor(WHITE);
    M5.Display.drawString("Digite no teclado abaixo:", 45, 115);
    M5.Display.drawRect(45, 145, 560, 185, TFT_DARKGRAY);

    // Painel 2: Sensores & Touch (Superior Direito)
    M5.Display.drawRect(650, 70, 600, 280, TFT_DARKGRAY);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString("[SENSORES: IMU BMI270 & TOUCH]", 665, 82);
    M5.Display.drawCircle(945, 260, 45, TFT_DARKGRAY);

    // Painel 3: Diagnóstico Geral (Inferior)
    M5.Display.drawRect(30, 370, 1220, 320, TFT_DARKGRAY);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString("[DIAGNOSTICO GERAL DE HARDWARE]", 45, 385);
    M5.Display.setTextColor(TFT_LIGHTGRAY);
    M5.Display.drawString("M5Stack Tab5 - ESP32-P4 v1.3 + ST7121 MIPI-DSI (1280x720) + STM32 Keyboard", 45, 620);
    M5.Display.drawString("Sistema Cooperativo Sem RTOS - Modelo de Referencia MaiaOS", 45, 650);

    // 3. Inicializa I2C do Teclado Físico (GPIO 0/1 @ 400kHz)
    Wire.begin(KEYBOARD_SDA, KEYBOARD_SCL, 400000);
    delay(50);
    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x10); // Working Mode
    Wire.write(0x02); // Character Mode
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x00); // INT_CFG
    Wire.write(0x07); // Habilita interrupções
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x02); // Limpa fila EVENT_NUM
    Wire.write(0x00);
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x01); // Limpa INT_STA
    Wire.write(0x00);
    Wire.endTransmission();

    // 4. Inicializa Áudio (Speaker e Mic)
    M5.Speaker.begin();
    M5.Speaker.setVolume(120);
    M5.Mic.begin();

    // Beep curto para confirmar som funcionando
    M5.Speaker.tone(1200, 150);

    // 5. Inicializa MicroSD (SDIO 4-bits)
    SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3);
    if (SD_MMC.begin("/sdcard", false, false, 20000)) {
        sdMontado = true;
        sdTotalMB = SD_MMC.totalBytes() / (1024 * 1024);
    }

    // 6. Varredura rápida de Wi-Fi via co-processador ESP32-C6
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    redesWiFiEncontradas = WiFi.scanNetworks(false, false, false, 250);

    // 7. Calibração inicial de CPU Load (100ms de loops livres)
    unsigned long calibraInicio = millis();
    while (millis() - calibraInicio < 100) {
        ciclosOciosoMax++;
    }
    ciclosOciosoMax *= 10; // Projeção para 1 segundo

    // 8. Cadastro de Tarefas no Scheduler Cooperativo
    scheduler.push_back({ tarefaAudio,               10, 0, true }); // Áudio contínuo
    scheduler.push_back({ tarefaTeclado,             40, 0, true }); // Varredura do teclado I2C
    scheduler.push_back({ tarefaTouch,               40, 0, true }); // Leitura do painel de toque
    scheduler.push_back({ tarefaSensoresIMU,         50, 0, true }); // Leitura do acelerômetro BMI270
    scheduler.push_back({ tarefaAtualizaSensores,    66, 0, true }); // Atualização dos sensores (~15 FPS)
    scheduler.push_back({ tarefaPiscaIndicador,     500, 0, true }); // LED indicador de status
    scheduler.push_back({ tarefaTelemetriaDisplay,  1000, 0, true }); // Painel de telemetria (1s)
    scheduler.push_back({ tarefaCalculaCPULoad,     1000, 0, true }); // Carga de processador (1s)
}

// ====================================================================
// LOOP PRINCIPAL
// ====================================================================
void loop() {
    M5.update();
    contagemCiclos++;

    unsigned long tempoAtual = millis();
    for (auto &t : scheduler) {
        if (t.ativa && (tempoAtual - t.tempoAnterior >= t.intervalo)) {
            t.tempoAnterior = tempoAtual;
            t.funcao();
        }
    }
}

// ====================================================================
// TAREFAS COOPERATIVAS
// ====================================================================

// TAREFA 1: Varredura do teclado físico I2C (STM32 A164)
void tarefaTeclado() {
    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x40);
    if (Wire.endTransmission(false) != 0) return;
    if (Wire.requestFrom(TAB5_KEYBOARD_ADDR, 1) != 1) return;
    uint8_t len = Wire.read();
    if (len == 0) return;

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x50);
    if (Wire.endTransmission(false) != 0) return;
    if (Wire.requestFrom(TAB5_KEYBOARD_ADDR, (uint8_t)(len + 1)) != (uint8_t)(len + 1)) return;

    Wire.read(); // Byte 0: modifier

    for (uint8_t i = 0; i < len; i++) {
        char c = (char)Wire.read();
        if (c == 0x08 || c == 127) { // Backspace
            if (textoTeclado.length() > 0) {
                textoTeclado.remove(textoTeclado.length() - 1);
                precisaAtualizarTeclado = true;
            }
        } else if (c == 0x0D || c == 0x0A) { // Enter
            textoTeclado = "";
            precisaAtualizarTeclado = true;
        } else if (c >= 32 && c <= 126) {
            if (textoTeclado == "Tab5 pronto para digitar...") {
                textoTeclado = "";
            }
            if (textoTeclado.length() < 40) {
                textoTeclado += c;
                precisaAtualizarTeclado = true;
            }
        }
    }

    // Limpa interrupção
    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x01);
    Wire.write(0x00);
    Wire.endTransmission();

    // Renderiza apenas quando houver alteração
    if (precisaAtualizarTeclado) {
        precisaAtualizarTeclado = false;
        M5.Display.fillRect(47, 147, 556, 180, BLACK);
        M5.Display.setCursor(55, 160);
        M5.Display.setTextSize(2.5);
        M5.Display.setTextColor(YELLOW);
        M5.Display.print(textoTeclado);
    }
}

// TAREFA 2: Touchscreen capacitivo
void tarefaTouch() {
    if (M5.Touch.getCount() > 0) {
        auto d = M5.Touch.getDetail(0);
        if (d.isPressed()) {
            touchX = d.x;
            touchY = d.y;
        }
    }
}

// TAREFA 3: Acelerômetro e Giroscópio (IMU BMI270)
void tarefaSensoresIMU() {
    if (M5.Imu.isEnabled()) {
        M5.Imu.getAccel(&accelX, &accelY, &accelZ);
        M5.Imu.getGyro(&gyroX, &gyroY, &gyroZ);
    }
}

// TAREFA 4: Áudio
void tarefaAudio() {
    if (audioPassThroughAtivo && M5.Mic.isEnabled() && M5.Speaker.isEnabled()) {
        size_t lidos = M5.Mic.record(bufferAudio, AUDIO_BUF_SIZE, 16000);
        if (lidos > 0) {
            M5.Speaker.playRaw(bufferAudio, lidos / sizeof(int16_t), 16000);
        }
    }
}

// TAREFA 5: Atualiza o painel de Sensores (IMU + Touch + Horizonte Artificial)
void tarefaAtualizaSensores() {
    M5.Display.setTextSize(2);
    
    // Telemetria numérica do IMU
    M5.Display.fillRect(665, 115, 570, 65, BLACK);
    M5.Display.setTextColor(WHITE);
    M5.Display.setCursor(665, 115);
    M5.Display.printf("Accel X: %+.2f  Y: %+.2f  Z: %+.2f G", accelX, accelY, accelZ);
    M5.Display.setCursor(665, 145);
    M5.Display.printf("Gyro  X: %+.1f  Y: %+.1f  Z: %+.1f d/s", gyroX, gyroY, gyroZ);

    // Status do Touch
    M5.Display.fillRect(665, 185, 570, 25, BLACK);
    M5.Display.setCursor(665, 185);
    if (touchX >= 0) {
        M5.Display.setTextColor(GREEN);
        M5.Display.printf("Ultimo Toque: X = %d, Y = %d", touchX, touchY);
    } else {
        M5.Display.setTextColor(TFT_LIGHTGRAY);
        M5.Display.print("Toque na tela para registrar");
    }

    // Mini horizonte artificial (bolinha de nível)
    int novoX = 945 - (int)(accelY * 40.0);
    int novoY = 260 - (int)(accelX * 40.0);
    if (novoX < 905) novoX = 905; if (novoX > 985) novoX = 985;
    if (novoY < 220) novoY = 220; if (novoY > 300) novoY = 300;

    if (novoX != bolaNivelX || novoY != bolaNivelY) {
        M5.Display.fillCircle(bolaNivelX, bolaNivelY, 8, BLACK);
        M5.Display.drawCircle(945, 260, 45, TFT_DARKGRAY);
        M5.Display.fillCircle(novoX, novoY, 8, ORANGE);
        bolaNivelX = novoX;
        bolaNivelY = novoY;
    }
}

// TAREFA 6: Atualiza o Painel Inferior de Telemetria (a cada 1s)
void tarefaTelemetriaDisplay() {
    tensaoBat = M5.Power.getBatteryVoltage();
    carregando = M5.Power.isCharging();

    M5.Display.setTextSize(2);

    // Linha 1: CPU Load e Uptime
    M5.Display.fillRect(45, 420, 1170, 30, BLACK);
    M5.Display.setTextColor(WHITE);
    M5.Display.setCursor(45, 420);
    M5.Display.printf("Carga da CPU: %.1f%%  |  Uptime: %lu s", cargaCPU, millis() / 1000);

    // Linha 2: Bateria
    M5.Display.fillRect(45, 460, 1170, 30, BLACK);
    M5.Display.setCursor(45, 460);
    M5.Display.printf("Bateria: %.2f V  (%s)", tensaoBat, carregando ? "Carregando" : "Bateria/Cheia");

    // Linha 3: MicroSD
    M5.Display.fillRect(45, 500, 1170, 30, BLACK);
    M5.Display.setCursor(45, 500);
    if (sdMontado) {
        M5.Display.setTextColor(GREEN);
        M5.Display.printf("MicroSD (SDIO): Montado com Sucesso (%llu MB)", sdTotalMB);
    } else {
        M5.Display.setTextColor(RED);
        M5.Display.print("MicroSD (SDIO): Nao inserido ou nao reconhecido");
    }

    // Linha 4: Wi-Fi
    M5.Display.fillRect(45, 540, 1170, 30, BLACK);
    M5.Display.setCursor(45, 540);
    if (redesWiFiEncontradas >= 0) {
        M5.Display.setTextColor(GREEN);
        M5.Display.printf("Wi-Fi (ESP32-C6): Ativo (%d redes detectadas no ar)", redesWiFiEncontradas);
    } else {
        M5.Display.setTextColor(YELLOW);
        M5.Display.print("Wi-Fi (ESP32-C6): Escaneando...");
    }
}

// TAREFA 7: Medidor de carga da CPU
void tarefaCalculaCPULoad() {
    if (contagemCiclos > ciclosOciosoMax) ciclosOciosoMax = contagemCiclos;
    float taxaOciosa = (float)contagemCiclos / (float)ciclosOciosoMax;
    cargaCPU = (1.0f - taxaOciosa) * 100.0f;
    if (cargaCPU < 0.0f) cargaCPU = 0.0f;
    if (cargaCPU > 100.0f) cargaCPU = 100.0f;
    contagemCiclos = 0;
}

// TAREFA 8: Indicador de atividade (Círculo no topo direito)
void tarefaPiscaIndicador() {
    ledPisca = !ledPisca;
    M5.Display.fillCircle(1220, 35, 12, ledPisca ? GREEN : DARKGREEN);
}
