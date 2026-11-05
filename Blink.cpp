#include <M5Unified.h>

unsigned long tempoAnteriorBateria = 0;
const long intervaloBateria = 2000; // Atualiza a cada 2 segundos

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    
    M5.Display.setTextSize(2);
    M5.Display.drawString("Monitor de Sistema (Sem RTOS)", 20, 20);
}

void loop() {
    M5.update(); // Mantém os botões e sensores internos atualizados
    
    unsigned long tempoAtual = millis();

    // Executa a leitura da bateria sem travar o resto do código
    if (tempoAtual - tempoAnteriorBateria >= intervaloBateria) {
        tempoAnteriorBateria = tempoAtual;

        // Limpa a região dos dados
        M5.Display.fillRect(20, 80, 500, 80, BLACK);
        M5.Display.setTextColor(YELLOW);

        // Captura dados de energia através do M5Unified
        float voltagem = M5.Power.getBatteryVoltage();
        
        M5.Display.setCursor(20, 80);
        M5.Display.printf("Tensao da Bateria: %.2f V", voltagem);
        
        if (M5.Power.isCharging()) {
            M5.Display.setCursor(20, 120);
            M5.Display.print("Status: Carregando...");
        } else {
            M5.Display.setCursor(20, 120);
            M5.Display.print("Status: Descarregando ou Cheia");
        }
    }

    // Você pode colocar qualquer outro código rápido aqui embaixo 
    // e ele rodará continuamente sem interrupções!
}
