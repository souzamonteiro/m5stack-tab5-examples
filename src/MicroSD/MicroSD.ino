#include <M5Unified.h>
#include <FS.h>
#include <SD_MMC.h>

// Definição exata dos pinos de alta velocidade (SDIO 4-bits) do M5Stack Tab5
#define SD_D0  GPIO_NUM_39
#define SD_D1  GPIO_NUM_40
#define SD_D2  GPIO_NUM_41
#define SD_D3  GPIO_NUM_42
#define SD_CLK GPIO_NUM_43
#define SD_CMD GPIO_NUM_44

const char* caminhoArquivo = "/teste_tab5.txt";

void setup() {
    // 1. Inicializa o ecossistema básico do M5Stack
    auto cfg = M5.config();
    M5.begin(cfg);

    // Configuração visual inicial da tela
    M5.Display.setBrightness(128);
    M5.Display.clear();
    M5.Display.setTextSize(2.5);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString("Sistema de Arquivos - Tab5 MicroSD", 20, 20);
    M5.Display.setTextColor(WHITE);

    // 2. Configura os pinos SDIO específicos do hardware do Tab5
    SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3);

    // 3. Monta o cartão SD (Velocidade padrão de 20MHz para estabilidade)
    M5.Display.setCursor(20, 70);
    M5.Display.print("Montando o cartao MicroSD... ");
    
    if (!SD_MMC.begin("/sdcard", false, false, 20000)) {
        M5.Display.setTextColor(RED);
        M5.Display.println("FALHOU!");
        M5.Display.drawString("Insira um cartao formatado em FAT32/exFAT.", 20, 110);
        while (1); // Trava o código em caso de erro crítico física/hardware
    }
    M5.Display.setTextColor(GREEN);
    M5.Display.println("OK!");
    M5.Display.setTextColor(WHITE);

    // ==========================================
    // ETAPA 1: GRAVAÇÃO (ESCRITA NO CARTÃO)
    // ==========================================
    M5.Display.setCursor(20, 110);
    M5.Display.printf("Abrindo %s para escrita... ", caminhoArquivo);

    // Abre o arquivo substituindo qualquer conteúdo anterior ("w" = write)
    File arquivoEscrita = SD_MMC.open(caminhoArquivo, FILE_WRITE);

    if (arquivoEscrita) {
        // Grava duas linhas de strings no arquivo
        arquivoEscrita.println("Ola do M5Stack Tab5!");
        arquivoEscrita.println("Gravacao efetuada com sucesso sem usar FreeRTOS.");
        arquivoEscrita.printf("Tempo de boot ate aqui: %lu ms\n", millis());
        
        arquivoEscrita.close(); // Sempre feche o arquivo para salvar os dados fisicamente
        M5.Display.setTextColor(GREEN);
        M5.Display.println("OK!");
    } else {
        M5.Display.setTextColor(RED);
        M5.Display.println("ERRO!");
    }

    // Pequena pausa apenas visual para acompanhar as etapas na tela
    delay(1000); 

    // ==========================================
    // ETAPA 2: LEITURA (IMPRIMIR NO LCD)
    // ==========================================
    M5.Display.setTextColor(WHITE);
    M5.Display.setCursor(20, 160);
    M5.Display.println("Lendo o arquivo gravado:");
    
    // Desenha uma borda cinza para destacar o conteúdo lido do SD
    M5.Display.drawRect(15, 190, 700, 200, DARKGRAY);
    M5.Display.setCursor(25, 200);
    M5.Display.setTextColor(YELLOW);

    // Abre o arquivo apenas para leitura ("r" = read)
    File arquivoLeitura = SD_MMC.open(caminhoArquivo, FILE_READ);

    if (arquivoLeitura) {
        // Enquanto houver bytes disponíveis no arquivo, lê caractere por caractere
        while (arquivoLeitura.available()) {
            char c = arquivoLeitura.read();
            
            // Corrige quebra de linha para o cursor do M5GFX avançar adequadamente
            if (c == '\n') {
                M5.Display.println();
                M5.Display.print(" "); // Margem interna esquerda
            } else if (c != '\r') {
                M5.Display.print(c);
            }
        }
        arquivoLeitura.close();
    } else {
        M5.Display.setTextColor(RED);
        M5.Display.println("Erro ao abrir o arquivo para leitura!");
    }
}

void loop() {
    // O loop fica livre. Toda a lógica rodou sequencialmente uma única vez no setup.
    M5.update();
    delay(10);
}
