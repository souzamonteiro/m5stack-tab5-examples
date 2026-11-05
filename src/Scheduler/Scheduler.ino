#include <M5Unified.h>
#include <vector>

// 1. Definição da Estrutura de uma Tarefa do Sistema
struct Tarefa {
    void (*funcao)();             // Ponteiro para a função que será executada
    unsigned long intervalo;       // Intervalo de execução em milissegundos
    unsigned long tempoAnterior;  // Armazena a última vez que a tarefa rodou
    bool ativa;                   // Permite ligar/desligar a tarefa dinamicamente
};

// Vetor dinâmico que armazenará todas as tarefas cadastradas no Scheduler
std::vector<Tarefa> scheduler;

// --- PROTÓTIPOS DAS FUNÇÕES QUE SERÃO NOSSAS TAREFAS ---
void tarefaAtualizaRelogio();
void tarefaLeituraTouch();
void tarefaVerificaSistema();

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setBrightness(128);
    M5.Display.clear();
    M5.Display.setTextSize(2.5);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawString("Scheduler Dinamico por Vetores (Sem RTOS)", 20, 20);
    M5.Display.drawFastHLine(20, 55, 760, TFT_DARKGRAY);

    // 2. CADASTRO DE TAREFAS NO SCHEDULER
    // Sintaxe: { ponteiro_funcao, intervalo_ms, tempo_inicial_0, ativa_true/false }
    scheduler.push_back({ tarefaAtualizaRelogio, 1000, 0, true });  // A cada 1 segundo
    scheduler.push_back({ tarefaLeituraTouch,      50, 0, true });  // Rápida: a cada 50ms
    scheduler.push_back({ tarefaVerificaSistema, 5000, 0, true });  // Lenta: a cada 5 segundos
}

void loop() {
    M5.update(); // Mantém os sensores e periféricos atualizados
    
    unsigned long tempoAtual = millis();

    // 3. O DESPACHANTE (Mecanismo do Scheduler)
    // Percorre todo o vetor de tarefas usando um Iterador/Referência
    for (auto &tarefa : scheduler) {
        // Verifica se a tarefa está ativa e se o tempo dela já expirou
        if (tarefa.ativa && (tempoAtual - tarefa.tempoAnterior >= tarefa.intervalo)) {
            tarefa.tempoAnterior = tempoAtual; // Atualiza o marcador de tempo
            tarefa.funcao();                   // Executa a função associada
        }
    }
}

// ====================================================================
// IMPLEMENTAÇÃO DAS TAREFAS REGISTRADAS
// ====================================================================

// TAREFA 1: Atualiza um cronômetro na tela (Simula exibição de relógio)
void tarefaAtualizaRelogio() {
    M5.Display.setCursor(20, 80);
    M5.Display.setTextColor(YELLOW, BLACK); // Texto amarelo com fundo preto para evitar fantasmas
    M5.Display.printf("Uptime do Sistema: %lu s ", millis() / 1000);
}

// TAREFA 2: Varredura síncrona e rápida do painel de toque (Touch)
void tarefaLeituraTouch() {
    if (M5.Touch.getCount() > 0) {
        auto detail = M5.Touch.getDetail(0);
        if (detail.isPressed()) {
            M5.Display.fillRect(20, 130, 400, 40, BLACK);
            M5.Display.setCursor(20, 130);
            M5.Display.setTextColor(WHITE);
            M5.Display.printf("Touch detectado: X=%d, Y=%d", detail.x, detail.y);
        }
    }
}

// TAREFA 3: Monitora parâmetros físicos básicos do Tab5
void tarefaVerificaSistema() {
    float voltagem = M5.Power.getBatteryVoltage();
    
    M5.Display.fillRect(20, 200, 500, 80, BLACK);
    M5.Display.setCursor(20, 200);
    M5.Display.setTextColor(GREEN);
    M5.Display.printf("[SYS LOG] Tensao da Bateria: %.2fV", voltagem);
    
    // Demonstração de controle dinâmico do Scheduler:
    // Se a voltagem cair abaixo de um limite crítico, poderíamos desativar tarefas pesadas
    // Exemplo: scheduler[1].ativa = false; // Desativaria a leitura do Touch
}
