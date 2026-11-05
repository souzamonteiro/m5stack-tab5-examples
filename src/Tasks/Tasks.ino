#include <M5Unified.h>
#include <Wire.h>
#include <WiFi.h>

// Configurações do Teclado e Wi-Fi
#define TAB5_KEYBOARD_ADDR 0x6D
const int8_t KEYBOARD_SDA = 0;
const int8_t KEYBOARD_SCL = 1;
const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA_WIFI";

// --- CONTROLE DE TEMPO DAS TAREFAS (Timers) ---
unsigned long tempoAnteriorTeclado = 0;
unsigned long tempoAnteriorPisca   = 0;
unsigned long tempoAnteriorWiFi    = 0;

const unsigned long intervaloTeclado = 50;   // 50 ms
const unsigned long intervaloPisca   = 500;  // 500 ms
const unsigned long intervaloWiFi    = 3000; // 3 segundos

// Variáveis globais de estado
String bufferTeclado = "";
bool estadoIndicador = false;

// --- DECLARAÇÃO DAS FUNÇÕES DAS TAREFAS ---
void tarefaLeituraTeclado();
void tarefaPiscaIndicador();
void tarefaMonitoraWiFi();

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(3); // Modo paisagem (1280 x 720) alinhado com o teclado físico
    Wire.begin(KEYBOARD_SDA, KEYBOARD_SCL, 400000);

    M5.Display.setBrightness(128);
    M5.Display.clear();
    M5.Display.setTextSize(2.5);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString("Multitarefa sem RTOS (millis)", 30, 20);

    // Inicializa conexão Wi-Fi de forma direta no setup
    M5.Display.setCursor(30, 60);
    M5.Display.setTextColor(WHITE);
    M5.Display.print("Conectando Wi-Fi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // Desenha as divisões estáticas da interface gráfica (1280x720)
    M5.Display.drawRect(30, 100, 600, 560, TFT_DARKGRAY); // Área do teclado
    M5.Display.drawString("Teclado:", 45, 115);

    // Coloca o teclado no modo caractere (0x02)
    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x10);
    Wire.write(0x02);
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x00);
    Wire.write(0x07);
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x02);
    Wire.write(0x00);
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x01);
    Wire.write(0x00);
    Wire.endTransmission();
}

void loop() {
    M5.update(); // Mantém o sistema e periféricos internos atualizados
    
    unsigned long tempoAtual = millis(); // Captura o tempo atual do processador

    // ---- DESPACHANTE DE TAREFAS (Scheduler manual) ----

    // Executa a Tarefa do Teclado se passaram 50ms
    if (tempoAtual - tempoAnteriorTeclado >= intervaloTeclado) {
        tempoAnteriorTeclado = tempoAtual;
        tarefaLeituraTeclado();
    }

    // Executa a Tarefa do Indicador se passaram 500ms
    if (tempoAtual - tempoAnteriorPisca >= intervaloPisca) {
        tempoAnteriorPisca = tempoAtual;
        tarefaPiscaIndicador();
    }

    // Executa a Tarefa do Wi-Fi se passaram 3000ms
    if (tempoAtual - tempoAnteriorWiFi >= intervaloWiFi) {
        tempoAnteriorWiFi = tempoAtual;
        tarefaMonitoraWiFi();
    }
}

// ====================================================================
// IMPLEMENTAÇÃO DAS TAREFAS (Sem nenhuma linha com delay!)
// ====================================================================

// TAREFA 1: Varredura do teclado físico I2C (Protocolo Tab5 A164)
void tarefaLeituraTeclado() {
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
            if (bufferTeclado.length() > 0) {
                bufferTeclado.remove(bufferTeclado.length() - 1);
                M5.Display.fillRect(45, 160, 570, 50, BLACK);
            }
        } else if (c == 0x0D || c == 0x0A) { // Enter
            bufferTeclado = "";
            M5.Display.fillRect(45, 160, 570, 50, BLACK);
        } else if (c >= 32 && c <= 126) {
            bufferTeclado += c;
        }
    }
    M5.Display.setCursor(45, 160);
    M5.Display.setTextColor(WHITE);
    M5.Display.print(bufferTeclado);

    // Limpa interrupção
    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x01);
    Wire.write(0x00);
    Wire.endTransmission();
}

// TAREFA 2: Animação visual na tela (Simula um LED piscando)
void tarefaPiscaIndicador() {
    estadoIndicador = !estadoIndicador; // Inverte o estado logico
    uint16_t cor = estadoIndicador ? GREEN : DARKGREEN;
    
    // Desenha um círculo preenchido no canto superior direito (1280px)
    M5.Display.fillCircle(1220, 35, 15, cor);
}

// TAREFA 3: Monitora o sinal do Wi-Fi e atualiza dados na tela (1280x720)
void tarefaMonitoraWiFi() {
    M5.Display.fillRect(661, 101, 588, 558, BLACK);
    M5.Display.drawRect(660, 100, 590, 560, TFT_DARKGRAY);
    M5.Display.setCursor(680, 125);
    
    if (WiFi.status() == WL_CONNECTED) {
        M5.Display.setTextColor(GREEN);
        M5.Display.println("Wi-Fi: CONECTADO");
        M5.Display.setCursor(680, 170);
        M5.Display.setTextColor(WHITE);
        M5.Display.printf("Sinal: %d dBm", WiFi.RSSI());
        M5.Display.setCursor(680, 215);
        M5.Display.print(WiFi.localIP().toString());
    } else {
        M5.Display.setTextColor(RED);
        M5.Display.println("Wi-Fi: DESCONECTADO");
    }
}
