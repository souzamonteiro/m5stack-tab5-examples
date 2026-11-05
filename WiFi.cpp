#include <M5Unified.h>
#include <WiFi.h>

// Substitua pelas credenciais da sua rede local
const char* ssid = "NOME_DA_SUA_REDE";
const char* password = "SENHA_DA_SUA_REDE";

void setup() {
    // 1. Inicializa o Tab5 (Tela, energia, etc.)
    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setBrightness(128);
    M5.Display.clear();
    M5.Display.setTextSize(2.5);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString("Gerenciador Wi-Fi (ESP32-C6 Co-Processor)", 20, 20);
    M5.Display.setTextColor(WHITE);

    // 2. Coloca o Wi-Fi no modo Estação (STA) para buscar e conectar a roteadores
    M5.Display.setCursor(20, 70);
    M5.Display.println("Configurando modo Estacao (STA)...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // Garante que não há conexões residuais antigas
    delay(500);

    // ==========================================
    // ETAPA 1: ESCANEAMENTO DE REDES (SCAN)
    // ==========================================
    M5.Display.setTextColor(YELLOW);
    M5.Display.println("Escaneando redes ao redor...");
    
    // O parâmetro 'false' faz o scan ser síncrono (bloqueante controlado, sem travar o chip)
    int n = WiFi.scanNetworks(false, false, false, 300); 
    
    if (n == 0) {
        M5.Display.setTextColor(RED);
        M5.Display.println("Nenhuma rede Wi-Fi encontrada.");
    } else {
        M5.Display.setTextColor(GREEN);
        M5.Display.printf("%d redes encontradas:\n", n);
        M5.Display.setTextSize(2);
        M5.Display.setTextColor(LIGHTGRAY);

        // Exibe as primeiras 5 redes para não estourar o tamanho vertical do LCD
        int limiteExibicao = (n > 5) ? 5 : n;
        for (int i = 0; i < limiteExibicao; ++i) {
            M5.Display.printf(" -> %s (%d dBm)\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        }
    }

    M5.Display.setTextSize(2.5);
    delay(2000); // Pausa para leitura dos dados do Scan

    // ==========================================
    // ETAPA 2: CONEXÃO COM A REDE DEFINIDA
    // ==========================================
    M5.Display.setTextColor(WHITE);
    M5.Display.println("\n----------------------------------");
    M5.Display.printf("Conectando a: %s\n", ssid);
    
    WiFi.begin(ssid, password);

    // Loop tradicional Arduino aguardando a conexão (substitui o vTaskDelay do RTOS)
    unsigned long timeoutInicial = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Display.print(".");
        
        // Timeout de segurança após 15 segundos tentando conectar
        if (millis() - timeoutInicial > 15000) {
            M5.Display.setTextColor(RED);
            M5.Display.println("\nFalha: Tempo limite de conexao esgotado!");
            while(1); // Para a execução se errar a senha ou o roteador sumir
        }
    }

    // Sucesso! Mostra as informações de rede na tela de 5"
    M5.Display.setTextColor(GREEN);
    M5.Display.println("\nConectado com sucesso!");
    
    M5.Display.setTextColor(WHITE);
    M5.Display.print("Endereco IP local: ");
    M5.Display.setTextColor(YELLOW);
    M5.Display.println(WiFi.localIP().toString());
}

void loop() {
    M5.update();

    // Monitoramento contínuo do status do sinal (RSSI) diretamente no loop sequencial
    if (WiFi.status() == WL_CONNECTED) {
        // Cria uma pequena "barra de status" dinâmica no rodapé do display
        M5.Display.fillRect(20, 360, 400, 40, BLACK);
        M5.Display.setCursor(20, 360);
        M5.Display.setTextSize(2);
        M5.Display.setTextColor(GREEN);
        M5.Display.printf("Sinal Wi-Fi Atual: %d dBm", WiFi.RSSI());
    } else {
        M5.Display.fillRect(20, 360, 400, 40, BLACK);
        M5.Display.setCursor(20, 360);
        M5.Display.setTextColor(RED);
        M5.Display.print("Conexao Perdida!");
    }

    delay(2000); // Atualiza a leitura do sinal a cada 2 segundos
}
