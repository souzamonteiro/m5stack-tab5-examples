#include <M5Unified.h>

// Coordenadas da bolinha móvel na tela (1280 x 720)
float bolaX = 640.0;
float bolaY = 360.0;
float velX = 0.0;
float velY = 0.0;
const float raioBola = 25.0;
const float atrito = 0.98; // Amortecimento suave

unsigned long ultimoTempo = 0;

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(3); // Paisagem alinhado ao teclado fisico

    M5.Display.setBrightness(128);
    M5.Display.clear();
    M5.Display.setTextSize(2.5);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString("M5Stack Tab5 - Acelerometro / IMU (BMI270)", 30, 20);

    // Desenha as bordas da arena de física
    M5.Display.drawRect(30, 100, 1220, 580, TFT_DARKGRAY);

    if (!M5.Imu.isEnabled()) {
        M5.Display.setTextColor(RED);
        M5.Display.drawString("IMU nao detectado!", 30, 60);
    } else {
        M5.Display.setTextColor(GREEN);
        M5.Display.drawString("Incline o Tab5 para mover a bolinha!", 30, 60);
    }

    ultimoTempo = millis();
}

void loop() {
    M5.update();

    unsigned long agora = millis();
    float dt = (agora - ultimoTempo) / 1000.0;
    if (dt <= 0.0 || dt > 0.1) dt = 0.02; // Limite de estabilidade
    ultimoTempo = agora;

    float ax = 0, ay = 0, az = 0;
    float gx = 0, gy = 0, gz = 0;

    M5.Imu.getAccel(&ax, &ay, &az);
    M5.Imu.getGyro(&gx, &gy, &gz);

    // Em rotacao 3 (paisagem):
    // A inclinacao horizontal e vertical mapeia a forca da gravidade:
    // ax acelera no eixo horizontal e ay no eixo vertical (com escala de sensibilidade)
    float forcaX = -ay * 600.0; // Inclinacao lateral
    float forcaY = -ax * 600.0; // Inclinacao frontal/traseira

    velX = (velX + forcaX * dt) * atrito;
    velY = (velY + forcaY * dt) * atrito;

    // Guarda posicao anterior para apagar apenas onde a bolinha estava
    float velhaX = bolaX;
    float velhaY = bolaY;

    bolaX += velX * dt;
    bolaY += velY * dt;

    // Colisão com as bordas da arena (30 a 1250 no X, 100 a 680 no Y)
    const float minX = 30 + raioBola + 2;
    const float maxX = 1250 - raioBola - 2;
    const float minY = 100 + raioBola + 2;
    const float maxY = 680 - raioBola - 2;

    if (bolaX < minX) { bolaX = minX; velX = -velX * 0.6; }
    if (bolaX > maxX) { bolaX = maxX; velX = -velX * 0.6; }
    if (bolaY < minY) { bolaY = minY; velY = -velY * 0.6; }
    if (bolaY > maxY) { bolaY = maxY; velY = -velY * 0.6; }

    // Apaga a posição anterior da bolinha e desenha na nova posição
    if ((int)velhaX != (int)bolaX || (int)velhaY != (int)bolaY) {
        M5.Display.fillCircle((int)velhaX, (int)velhaY, (int)raioBola, BLACK);
        M5.Display.fillCircle((int)bolaX, (int)bolaY, (int)raioBola, ORANGE);
        M5.Display.drawCircle((int)bolaX, (int)bolaY, (int)raioBola, YELLOW);
    }

    // Painel de telemetria numérica no topo
    static unsigned long ultimoDisplay = 0;
    if (agora - ultimoDisplay >= 100) {
        ultimoDisplay = agora;
        M5.Display.fillRect(750, 20, 500, 70, BLACK);
        M5.Display.setTextSize(2);
        M5.Display.setTextColor(WHITE);
        M5.Display.setCursor(750, 25);
        M5.Display.printf("Accel: X:%+.2f Y:%+.2f Z:%+.2f G", ax, ay, az);
        M5.Display.setCursor(750, 55);
        M5.Display.setTextColor(TFT_LIGHTGRAY);
        M5.Display.printf("Gyro : X:%+.1f Y:%+.1f Z:%+.1f d/s", gx, gy, gz);
    }

    // Se o usuário tocar na tela, recentraliza a bolinha
    if (M5.Touch.getCount() > 0) {
        auto detail = M5.Touch.getDetail(0);
        if (detail.isPressed()) {
            M5.Display.fillCircle((int)bolaX, (int)bolaY, (int)raioBola, BLACK);
            bolaX = detail.x;
            bolaY = detail.y;
            velX = 0;
            velY = 0;
        }
    }

    delay(10); // ~100 FPS
}
