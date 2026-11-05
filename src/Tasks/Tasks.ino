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
    Wire.begin(KEYBOARD_SDA, KEYBOARD_SCL, 400000);

    M5.Display.setBrightness(128);
    M5.Display.clear();
    M5.Display.setTextSize(2.5);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString("Multitarefa sem RTOS (millis)", 20, 20);

    // Inicializa conexão Wi-Fi de forma direta no setup
    M5.Display.setCursor(20, 60);
    M5.Display.print("Conectando Wi-Fi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // Desenha as divisões estáticas da interface gráfica
    M5.Display.drawRect(15, 100, 450, 150, DARKGRAY); // Área do teclado
    M5.Display.drawString("Teclado:", 30, 110, &fonts::Font0);
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

// TAREFA 1: Varredura rápida do teclado físico I2C
void tarefaLeituraTeclado() {
    Wire.requestFrom(TAB5_KEYBOARD_ADDR, 1);
    if (Wire.available()) {
        char c = Wire.read();
        if (c != 0x00) {
            if (c == 0x08 || c == 127) { // Backspace
                if (bufferTeclado.length() > 0) {
                    bufferTeclado.remove(bufferTeclado.length() - 1);
                    M5.Display.fillRect(30, 150, 420, 40, BLACK);
                }
            } else if (c == 0x0D || c == 0x0A) { // Enter
                bufferTeclado = "";
                M5.Display.fillRect(30, 150, 420, 40, BLACK);
            } else if (c >= 32 && c <= 126) {
                bufferTeclado += c;
            }
            M5.Display.setCursor(30, 150);
            M5.Display.setTextColor(WHITE);
            M5.Display.print(bufferTeclado);
        }
    }
}

// TAREFA 2: Animação visual na tela (Simula um LED piscando)
void tarefaPiscaIndicador() {
    estadoIndicador = !estadoIndicador; // Inverte o estado logico
    uint16_t cor = estadoIndicador ? GREEN : DARKGREEN;
    
    // Desenha um círculo preenchido no canto superior direito
    M5.Display.fillCircle(750, 30, 15, cor);
}

// TAREFA 3: Monitora o sinal do Wi-Fi e atualiza dados na tela
void tarefaMonitoraWiFi() {
    M5.Display.fillRect(490, 100, 280, 150, BLACK);
    M5.Display.drawRect(490, 100, 280, 150, DARKGRAY);
    M5.Display.setCursor(505, 115);
    
    if (WiFi.status() == WL_CONNECTED) {
        M5.Display.setTextColor(GREEN);
        M5.Display.println("Wi-Fi: CONECTADO");
        M5.Display.setCursor(505, 150);
        M5.Display.setTextColor(WHITE);
        M5.Display.printf("Sinal: %d dBm", WiFi.RSSI());
        M5.Display.setCursor(505, 185);
        M5.Display.print(WiFi.localIP().toString());
    } else {
        M5.Display.setTextColor(RED);
        M5.Display.println("Wi-Fi: DESCONECTADO");
    }
}
