#include <M5Unified.h>
#include <Wire.h>

// Endereço I2C oficial do teclado do Tab5 (STM32)
#define TAB5_KEYBOARD_ADDR 0x6D 

// Pinos I2C específicos do barramento inferior do Tab5 (Ext. Port 1)
const int8_t KEYBOARD_SDA = 0;
const int8_t KEYBOARD_SCL = 1;

String textoDigitado = ""; // Armazena o texto acumulado

void setup() {
    Serial.begin(115200);

    // Inicializa a tela e periféricos básicos do Tab5
    auto cfg = M5.config();
    M5.begin(cfg);

    // Inicializa o barramento I2C dedicado ao teclado
    Wire.begin(KEYBOARD_SDA, KEYBOARD_SCL, 400000);
    delay(50);

    // Configura o teclado Tab5 (STM32 @ 0x6D) em modo Caractere
    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x10); // Reg 0x10: Working Mode
    Wire.write(0x02); // 2 = Character Mode
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x00); // Reg 0x00: INT_CFG
    Wire.write(0x07); // Enable interrupts
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x02); // Reg 0x02: Clear EVENT_NUM queue
    Wire.write(0x00);
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x01); // Reg 0x01: Clear INT_STA
    Wire.write(0x00);
    Wire.endTransmission();

    // Configuração inicial da tela LCD do Tab5
    M5.Display.setBrightness(128);
    M5.Display.clear();
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(GREEN);
    M5.Display.drawString("Tab5 Keyboard Echo (Sem RTOS)", 20, 20);
    
    M5.Display.setTextColor(WHITE);
    M5.Display.setCursor(20, 100);
    M5.Display.print("Digite algo no teclado:");
}

void loop() {
    M5.update(); // Atualiza o estado geral do sistema

    // 1. Consulta o registrador 0x40 (Tamanho da string disponível na fila)
    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x40);
    if (Wire.endTransmission(false) == 0) {
        if (Wire.requestFrom((uint8_t)TAB5_KEYBOARD_ADDR, (uint8_t)1) == 1) {
            uint8_t qlen = Wire.read();

            if (qlen >= 1 && qlen <= 16) {
                // 2. Lê exatamente (qlen + 1) bytes a partir do registrador 0x50
                // Byte 0: Modificador (Ctrl, Alt, etc.)
                // Bytes 1..qlen: Conteúdo dos caracteres
                Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
                Wire.write(0x50);
                if (Wire.endTransmission(false) == 0) {
                    if (Wire.requestFrom((uint8_t)TAB5_KEYBOARD_ADDR, (uint8_t)(qlen + 1)) == (size_t)(qlen + 1)) {
                        uint8_t mod = Wire.read();
                        (void)mod;

                        String str = "";
                        while (Wire.available()) {
                            char c = (char)Wire.read();
                            if (c != 0) {
                                str += c;
                            }
                        }

                        // Reconhece teclas especiais nomeadas pelo firmware do teclado
                        String lower = str;
                        lower.toLowerCase();

                        if (lower == "enter" || lower == "return" || str == "\r" || str == "\n") {
                            textoDigitado = ""; // Limpa buffer para nova linha
                            M5.Display.fillRect(20, 150, 750, 50, BLACK);
                        } else if (lower == "backspace" || str == "\b" || (uint8_t)str[0] == 127) {
                            if (textoDigitado.length() > 0) {
                                textoDigitado.remove(textoDigitado.length() - 1);
                                M5.Display.fillRect(20, 150, 750, 50, BLACK);
                            }
                        } else if (lower == "space") {
                            textoDigitado += " ";
                        } else if (lower == "tab") {
                            textoDigitado += "  ";
                        } else {
                            // Caracteres comuns
                            for (size_t i = 0; i < str.length(); ++i) {
                                char c = str[i];
                                if (c >= 32 && c <= 126) {
                                    textoDigitado += c;
                                }
                            }
                        }

                        // Atualiza na tela
                        M5.Display.fillRect(20, 150, 750, 50, BLACK);
                        M5.Display.setCursor(20, 150);
                        M5.Display.print(textoDigitado);

                        // Limpa INT_STA para liberar a interrupção
                        Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
                        Wire.write(0x01);
                        Wire.write(0x00);
                        Wire.endTransmission();
                    }
                }
            }
        }
    }

    delay(20);
}
