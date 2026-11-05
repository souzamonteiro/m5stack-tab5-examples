#include <M5Unified.h>
#include <vector>
#include <FS.h>
#include <SD_MMC.h>
#include <lvgl.h>
#include <BluetoothSerial.h>

// --- CONFIGURAÇÕES DE HARDWARE ---
#define TAMANHO_BUFFER_AUDIO 1024
int16_t bufferAudio[TAMANHO_BUFFER_AUDIO];

// Instanciação de periféricos específicos
BluetoothSerial SerialBT;
LGFX_Sprite canvas(&M5.Display); // Nosso Canvas para Double Buffering

// --- VARIÁVEIS DO MEDIDOR DE CARGA (CPU LOAD) ---
unsigned long contagemCiclos = 0;
unsigned long ciclosOciosoMax = 0;
float cargaCPU = 0.0;
unsigned long tempoAnteriorCPU = 0;

// --- ESTRUTURA DO SCHEDULER ---
struct Tarefa {
    void (*funcao)();
    unsigned long intervalo;
    unsigned long tempoAnterior;
    bool ativa;
};
std::vector<Tarefa> scheduler;

// --- PROTÓTIPOS DAS TAREFAS ---
void tarefaGerenciaAudio();
void tarefaAtualizaSprites();
void tarefaMotorLVGL();
void tarefaCalculaCPULoad();
void tarefaGerenciaBluetooth();

// --- INICIALIZADOR DO LVGL (FLUSH DISPLAY) ---
void meu_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    M5.Display.pushImageDMA(area->x1, area->y1, w, h, (uint16_t *)&color_p->full);
    lv_disp_flush_ready(disp);
}

void setup() {
    // 1. Inicializa o ecossistema M5Unified com Áudio ativado
    auto cfg = M5.config();
    cfg.external_speaker.channels = 1; 
    M5.begin(cfg);

    // 2. Inicializa e Configura o Áudio (M5.Speaker e M5.Mic)
    M5.Speaker.begin();
    M5.Mic.begin();

    // 3. Inicializa o Bluetooth Clássico através do ESP32-C6
    SerialBT.begin("M5Tab5_Cooperativo");

    // 4. Configuração do Canvas (LGFX_Sprite) para Double Buffering
    // Criamos um canvas menor para renderizar telemetria sem flickering
    canvas.setColorDepth(16);
    canvas.createSprite(300, 100); 

    // 5. Inicialização da Interface LVGL Básica
    lv_init();
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[800 * 40]; // Buffer de linhas para renderização
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 800 * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 800;
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = meu_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Criando um label simples no LVGL para teste da interface
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Interface LVGL Ativa!");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

    // 6. Calibração inicial do CPU Load (Mede o pico de velocidade em repouso)
    unsigned long tempoCalibracao = millis();
    while(millis() - tempoCalibracao < 100) {
        ciclosOciosoMax++; // Registra quantos loops vazios ocorrem em 100ms
    }
    ciclosOciosoMax *= 10; // Escalona para 1 segundo de referência

    // 7. Registro de tarefas no nosso Scheduler Manual
    scheduler.push_back({ tarefaGerenciaAudio,       5, 0, true }); // Muito rápida (Streaming)
    scheduler.push_back({ tarefaMotorLVGL,          5, 0, true }); // Rápida para animações fluidas
    scheduler.push_back({ tarefaAtualizaSprites,   33, 0, true }); // ~30 FPS para o Canvas nativo
    scheduler.push_back({ tarefaGerenciaBluetooth, 100, 0, true }); // Checagem de dados seriais
    scheduler.push_back({ tarefaCalculaCPULoad,  1000, 0, true }); // A cada 1 segundo
}

void loop() {
    M5.update();
    contagemCiclos++; // Incrementa cada iteração livre do processador

    unsigned long tempoAtual = millis();

    // Loop do Despachante do Scheduler Cooperativo
    for (auto &tarefa : scheduler) {
        if (tarefa.ativa && (tempoAtual - tarefa.tempoAnterior >= tarefa.intervalo)) {
            tarefa.tempoAnterior = tempoAtual;
            tarefa.funcao();
        }
    }
}

// ====================================================================
// IMPLEMENTAÇÃO DAS TAREFAS SOLICITADAS (SEM BLOCK / DELAY)
// ====================================================================

// 1 & 2. REPRODUTOR (M5.Speaker) E GRAVADOR (M5.Mic)
// Captura o áudio do microfone e imediatamente repassa ao alto-falante (Modo Eco/Pass-through)
void tarefaGerenciaAudio() {
    if (M5.Mic.isEnabled() && M5.Speaker.isEnabled()) {
        // Verifica se há dados capturados no microfone via DMA
        size_t bytesLidos = M5.Mic.record(bufferAudio, TAMANHO_BUFFER_AUDIO, 16000);
        
        if (bytesLidos > 0) {
            // Reproduz instantaneamente o bloco de áudio capturado
            M5.Speaker.playRaw(bufferAudio, bytesLidos / sizeof(int16_t), 16000);
        }
    }
}

// 3. USO DE CANVASES / SPRITES (LGFX_Sprite)
// Desenha dados dinâmicos na memória RAM interna e joga na tela de uma vez, evitando flickering
void tarefaAtualizaSprites() {
    canvas.clear(BLACK);
    canvas.setTextSize(2);
    
    // Renderiza textos dentro do canvas em memória
    canvas.setTextColor(CYAN);
    canvas.drawString("--- Telemetria Sprite ---", 10, 10);
    
    canvas.setTextColor(WHITE);
    canvas.printf("Carga da CPU: %.1f%%", cargaCPU);
    
    // Envia o canvas renderizado para uma coordenada específica do LCD do Tab5
    canvas.pushSprite(20, 350);
}

// 4. INTERFACE LVGL BÁSICA
// Executa o processamento interno de renderização do motor gráfico do LVGL
void tarefaMotorLVGL() {
    lv_timer_handler(); 
}

// 5. COMUNICAÇÃO BLUETOOTH
// Verifica se há mensagens via Bluetooth Clássico e imprime no terminal do computador
void tarefaGerenciaBluetooth() {
    if (SerialBT.available()) {
        char caractereBT = SerialBT.read();
        Serial.print("Recebido via BT: ");
        Serial.println(caractereBT);
    }
}

// 6. MEDIDOR DE CARGA DO PROCESSADOR (CPU LOAD)
// Calcula a porcentagem de processamento baseada em ciclos perdidos por bloqueios de código
void tarefaCalculaCPULoad() {
    if (contagemCiclos > ciclosOciosoMax) ciclosOciosoMax = contagemCiclos; // Auto-ajuste adaptativo
    
    // Calcula a proporção de ciclos executados versus o potencial máximo ocioso
    float taxaOciosa = (float)contagemCiclos / (float)ciclosOciosoMax;
    cargaCPU = (1.0f - taxaOciosa) * 100.0f;
    
    // Garante limites comerciais entre 0% e 100%
    if (cargaCPU < 0) cargaCPU = 0;
    if (cargaCPU > 100) cargaCPU = 100;

    contagemCiclos = 0; // Reseta o contador para o próximo segundo
}
