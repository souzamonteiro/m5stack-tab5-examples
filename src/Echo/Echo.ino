#include <M5Unified.h>
#include <Wire.h>

// Endereço I2C oficial do teclado do Tab5
#define TAB5_KEYBOARD_ADDR 0x6D 

// Pinos I2C específicos do barramento inferior do Tab5 (Ext. Port 1)
const int8_t KEYBOARD_SDA = 0;
const int8_t KEYBOARD_SCL = 1;

String textoDigitado = ""; // Armazena o texto acumulado

void setup() {
    // Inicializa a tela e periféricos básicos do Tab5
    auto cfg = M5.config();
    M5.begin(cfg);

    // Inicializa o barramento I2C dedicado ao teclado
    Wire.begin(KEYBOARD_SDA, KEYBOARD_SCL, 400000);

    // Configuração inicial da tela LCD do Tab5
    M5.Display.setBrightness(128);
    M5.Display.clear();
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(GREEN);
    M5.Display.drawString("Tab5 Keyboard Echo (Sem RTOS)", 20, 20);
    
    M5.Display.setTextColor(WHITE);
    M5.Display.setCursor(20, 100);
    M5.Display.print("Digite algo: ");
}

void loop() {
    M5.update(); // Atualiza o estado geral do sistema

    // Solicita dados ao chip STM32 interno do teclado
    // O firmware em modo 'Character' padrão devolve os dados quando consultado
    Wire.requestFrom(TAB5_KEYBOARD_ADDR, 1); 

    if (Wire.available()) {
        char caractereRecendido = Wire.read();

        // 0x00 significa que nenhuma tecla foi pressionada neste ciclo
        if (caractereRecendido != 0x00) { 
            
            // Tratamento da tecla BACKSPACE (Código ASCII 0x08 ou 127 dependendo do modo)
            if (caractereRecendido == 0x08 || caractereRecendido == 127) {
                if (textoDigitado.length() > 0) {
                    // Remove o último caractere da String
                    textoDigitado.remove(textoDigitado.length() - 1);
                    
                    // Apaga a linha antiga na tela desenhando um retângulo preto
                    M5.Display.fillRect(20, 150, 750, 50, BLACK); 
                }
            } 
            // Tratamento da tecla ENTER (Nova Linha)
            else if (caractereRecendido == 0x0D || caractereRecendido == 0x0A) {
                textoDigitado = ""; // Limpa o buffer para uma nova digitação
                M5.Display.fillRect(20, 150, 750, 50, BLACK);
            } 
            // Adiciona caracteres visíveis comuns à string (ASCII imprimível)
            else if (caractereRecendido >= 32 && caractereRecendido <= 126) {
                textoDigitado += caractereRecendido;
            }

            // Renderiza o texto atualizado na tela
            M5.Display.setCursor(20, 150);
            M5.Display.print(textoDigitado);
        }
    }

    // Delay curto apenas para compassar a varredura do I2C e evitar flickering
    delay(30); 
}
