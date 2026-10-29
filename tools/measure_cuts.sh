# Measure the flash saved by disabling each option on its own. Usage: bash tools/measure_cuts.sh FILE:NAME ...
# FILE is C (Configuration.h) or A (Configuration_adv.h). Restores the config afterwards.
set -u
cd "$(dirname "$0")/.."
BIN=firmware/.pio/build/GD32F103RET6_sovol_maple
size() { ls -t $BIN/*.bin | head -1 | xargs stat -c %s; }
base=$(pio run -d firmware -s >/dev/null 2>&1 && size)
echo "baseline $base"
for spec in "$@"; do
  f=${spec%%:*}; name=${spec#*:}
  [ "$f" = C ] && path=firmware/Marlin/Configuration.h || path=firmware/Marlin/Configuration_adv.h
  cp "$path" "$path.bak"
  uv run --no-project tools/port_config.py set "$path" "-$name" >/dev/null
  if pio run -d firmware -s >/dev/null 2>&1; then s=$(size); echo "$name saves $((base - s))"; else echo "$name BUILD FAILED"; fi
  mv "$path.bak" "$path"
done
