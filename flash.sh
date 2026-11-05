#!/usr/bin/env bash
set -e

ARDUINO_CLI="/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli"
PORT="${PORT:-/dev/cu.usbmodem1301}"
FQBN="m5stack:esp32:m5stack_tab5"

EXAMPLE="${1:-Echo}"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH="$DIR/src/$EXAMPLE"

if [ ! -d "$SKETCH" ]; then
    echo "Erro: Exemplo '$EXAMPLE' não encontrado em $DIR/src/"
    echo "Exemplos disponíveis:"
    ls -1 "$DIR/src"
    exit 1
fi

echo "==> Compilando $EXAMPLE para M5Tab5..."
"$ARDUINO_CLI" compile -b "$FQBN" "$SKETCH"

echo "==> Gravando $EXAMPLE em $PORT..."
"$ARDUINO_CLI" upload -p "$PORT" -b "$FQBN" "$SKETCH"

echo "==> Sucesso! '$EXAMPLE' gravado no M5Stack Tab5."

