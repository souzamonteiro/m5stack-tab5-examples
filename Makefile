# ==============================================================================
# M5Stack Tab5 (ESP32-P4) Examples - Makefile
# ==============================================================================
# Usage:
#   make help                   - Exibe os comandos disponíveis
#   make compile SKETCH=Echo    - Compila um exemplo específico
#   make flash SKETCH=FullTest  - Compila e grava na placa
#   make <sketch-name>          - Atalho de compilação (ex: make FullTest)
#   make flash-<sketch-name>    - Atalho de gravação (ex: make flash-FullTest)
# ==============================================================================

SHELL := /bin/bash

# Detecta arduino-cli no PATH ou dentro do bundle oficial da Arduino IDE no macOS
ARDUINO_CLI ?= $(shell which arduino-cli 2>/dev/null || \
                 test -x "/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" && \
                 echo "/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" || \
                 echo "arduino-cli")

# FQBN padrão para M5Stack Tab5 com PSRAM ativada
FQBN ?= m5stack:esp32:m5stack_tab5:PSRAM=enabled

# Detecta automaticamente a porta serial conectada
PORT ?= $(shell ls /dev/cu.usbmodem* /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | head -n 1)

# Lista de todos os sketches disponíveis
SKETCHES := Blink Echo Touch Scheduler Tasks WiFi Movement MicroSD FullTest

.PHONY: all help list-ports clean $(SKETCHES) $(addprefix flash-,$(SKETCHES))

help:
	@echo "=================================================================="
	@echo "           M5Stack Tab5 (ESP32-P4) - Exemplos de Código           "
	@echo "=================================================================="
	@echo "Porta detectada: $(if $(PORT),$(PORT),Nenhuma porta encontrada)"
	@echo "arduino-cli:     $(ARDUINO_CLI)"
	@echo "FQBN:            $(FQBN)"
	@echo ""
	@echo "Exemplos disponíveis:"
	@for s in $(SKETCHES); do printf "  - %-12s (make %s | make flash-%s)\n" $$s $$s $$s; done
	@echo ""
	@echo "Comandos gerais:"
	@echo "  make all                     - Compila todos os exemplos"
	@echo "  make compile SKETCH=<nome>   - Compila o exemplo especificado"
	@echo "  make flash SKETCH=<nome>     - Compila e grava no Tab5"
	@echo "  make list-ports              - Lista as portas seriais conectadas"
	@echo "  make monitor                 - Abre o monitor serial (115200 baud)"
	@echo "  make clean                   - Limpa o cache de compilação"
	@echo "=================================================================="

# Regras individuais de compilação
$(SKETCHES):
	@echo "==> Compilando $@ para M5Stack Tab5..."
	@"$(ARDUINO_CLI)" compile --fqbn $(FQBN) src/$@/$@.ino

# Regras individuais de gravação (flash)
$(addprefix flash-,$(SKETCHES)):
	@sketch=$(subst flash-,,$@); \
	if [ -z "$(PORT)" ]; then \
	    echo "ERRO: Nenhuma porta serial detectada. Conecte o Tab5 ou passe PORT=/dev/..."; \
	    exit 1; \
	fi; \
	echo "==> Compilando $$sketch..."; \
	"$(ARDUINO_CLI)" compile --fqbn $(FQBN) src/$$sketch/$$sketch.ino && \
	echo "==> Gravando $$sketch em $(PORT)..." && \
	"$(ARDUINO_CLI)" upload -p $(PORT) --fqbn $(FQBN) src/$$sketch/$$sketch.ino && \
	echo "==> Sucesso! '$$sketch' gravado no M5Stack Tab5."

# Compilação genérica por variável SKETCH
compile:
	@if [ -z "$(SKETCH)" ]; then \
	    echo "Uso: make compile SKETCH=<NomeDoSketch>"; \
	    exit 1; \
	fi
	@"$(ARDUINO_CLI)" compile --fqbn $(FQBN) src/$(SKETCH)/$(SKETCH).ino

# Gravação genérica por variável SKETCH
flash:
	@if [ -z "$(SKETCH)" ]; then \
	    echo "Uso: make flash SKETCH=<NomeDoSketch>"; \
	    exit 1; \
	fi
	@$(MAKE) flash-$(SKETCH)

# Compila todos os sketches
all:
	@for s in $(SKETCHES); do \
	    echo ""; \
	    echo "================================================"; \
	    echo " Compilando $$s..."; \
	    echo "================================================"; \
	    "$(ARDUINO_CLI)" compile --fqbn $(FQBN) src/$$s/$$s.ino || exit 1; \
	done
	@echo ""
	@echo "==> Todos os $(words $(SKETCHES)) exemplos foram compilados com sucesso!"

list-ports:
	@"$(ARDUINO_CLI)" board list

monitor:
	@if [ -z "$(PORT)" ]; then \
	    echo "ERRO: Nenhuma porta serial detectada. Conecte o Tab5 ou passe PORT=/dev/..."; \
	    exit 1; \
	fi
	@"$(ARDUINO_CLI)" monitor -p $(PORT) -c baudrate=115200

clean:
	@echo "Limpando diretórios temporários de compilação..."
	@rm -rf /Users/roberto/Library/Caches/arduino/sketches/* 2>/dev/null || true
	@echo "Concluído."

