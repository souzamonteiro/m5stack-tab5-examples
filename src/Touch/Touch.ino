#include <M5Unified.h>

void setup() {
    // Inicializa o ecossistema M5Stack (Tela, Energia, Touch)
    auto cfg = M5.config();
    M5.begin(cfg);

    // Configuração inicial do display
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(GREEN);
    M5.Display.drawString("M5Stack Tab5 - Touch Demo", 20, 20);
}

void loop() {
    // Atualiza o estado interno dos periféricos (incluindo o Touch)
    M5.update();

    // Verifica se a tela está sendo tocada
    if (M5.Touch.getCount() > 0) {
        auto detail = M5.Touch.getDetail(0);
        
        if (detail.isPressed()) {
            // Limpa a área de texto e posiciona o cursor no mesmo lugar sempre
            M5.Display.fillRect(20, 100, 600, 50, BLACK);
            M5.Display.setCursor(20, 100);
            M5.Display.setTextColor(WHITE);
            
            // Exibe as coordenadas X e Y do toque
            M5.Display.printf("Toque detectado em X: %d, Y: %d", detail.x, detail.y);

            // Desenha um indicador onde o dedo está tocando
            M5.Display.fillCircle(detail.x, detail.y, 4, RED);
        }
    }
}
