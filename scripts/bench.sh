#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/titanx"
ITERATIONS="${1:-100}"

if [[ ! -x "$BIN" ]]; then
  echo "Building titanx..."
  make -C "$ROOT_DIR"
fi

measure_ms() {
  local start end
  start=$(date +%s%N)
  "$@" >/dev/null 2>&1
  end=$(date +%s%N)
  echo $(((end - start) / 1000000))
}

echo "Titan-X benchmark"
echo "Binary: $BIN"

a=0
startup_ms=$(measure_ms "$BIN" /bin/true)
echo "Startup + single command: ${startup_ms} ms"

start=$(date +%s%N)
for ((i = 0; i < ITERATIONS; i++)); do
  "$BIN" /bin/echo ok >/dev/null
  a=$((a + 1))
done
end=$(date +%s%N)

batch_ms=$(((end - start) / 1000000))
echo "${ITERATIONS} command executions: ${batch_ms} ms"
echo "Average per command: $((batch_ms * 1000 / ITERATIONS)) us"
echo "Executed count: $a"
