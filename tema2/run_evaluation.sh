#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

N_BROKERS=${1:-3}
N_SUBSCRIBERS=${2:-3}
N_PUBLISHERS=${3:-1}
DURATION_SEC=${4:-180}
TOTAL_SUBSCRIPTIONS=${5:-10000}
PUBLISH_DELAY_MS=${6:-10}
SETTLE_SECONDS=${7:-8}

SUBS_PER_SUBSCRIBER=$(( (TOTAL_SUBSCRIPTIONS + N_SUBSCRIBERS - 1) / N_SUBSCRIBERS ))
PUBLICATIONS_PER_PUBLISHER=$(( (DURATION_SEC * 1000) / PUBLISH_DELAY_MS ))
REPORT_DIR="$SCRIPT_DIR/logs/evaluation_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$REPORT_DIR"
REPORT="$REPORT_DIR/evaluation_report.md"

run_case() {
  local eq_pct="$1"
  local label="eq_${eq_pct}"
  local out_file="$REPORT_DIR/${label}.out"

  echo "=== Evaluare company EQ ${eq_pct}% ==="
  bash run_system.sh \
    "$N_BROKERS" \
    "$N_SUBSCRIBERS" \
    "$N_PUBLISHERS" \
    "$SUBS_PER_SUBSCRIBER" \
    "$PUBLICATIONS_PER_PUBLISHER" \
    "$eq_pct" \
    "$PUBLISH_DELAY_MS" \
    "$SETTLE_SECONDS" | tee "$out_file"

  local log_dir
  log_dir=$(grep "Log dir:" "$out_file" | tail -n 1 | sed 's/Log dir: //')

  local sent
  sent=$(grep "Publicatii trimise:" "$out_file" | awk '{print $3}')

  local unique_delivered
  unique_delivered=$(grep "Publicatii unice livrate:" "$out_file" | awk '{print $4}')

  local notifications
  notifications=$(grep "Notificari totale livrate:" "$out_file" | awk '{print $4}')

  local latency
  latency=$(grep "Latenta medie:" "$out_file" | awk '{print $3}')

  local match_rate
  match_rate=$(awk -v n="$notifications" -v s="$sent" -v subs="$((SUBS_PER_SUBSCRIBER * N_SUBSCRIBERS))" \
    'BEGIN { if (s * subs > 0) printf "%.6f", (100.0 * n) / (s * subs); else printf "0" }')

  {
    echo "| ${eq_pct}% | $sent | $unique_delivered | $notifications | ${latency} us | ${match_rate}% | [$label]($log_dir) |"
  } >> "$REPORT.tmp"
}

cat > "$REPORT" <<EOF
# Raport evaluare publish/subscribe

Configuratie:
- brokeri: $N_BROKERS
- subscriberi: $N_SUBSCRIBERS
- publisheri: $N_PUBLISHERS
- subscriptii cerute: $TOTAL_SUBSCRIPTIONS
- subscriptii generate: $((SUBS_PER_SUBSCRIBER * N_SUBSCRIBERS))
- durata feed: $DURATION_SEC secunde
- delay publisher: $PUBLISH_DELAY_MS ms
- timp stabilizare dupa inregistrarea subscriptiilor: $SETTLE_SECONDS secunde
- publicatii per publisher: $PUBLICATIONS_PER_PUBLISHER

| Company EQ | Publicatii trimise | Publicatii unice livrate | Notificari livrate | Latenta medie | Matching rate | Loguri |
|---:|---:|---:|---:|---:|---:|---|
EOF

: > "$REPORT.tmp"
run_case 100
run_case 25
cat "$REPORT.tmp" >> "$REPORT"
rm -f "$REPORT.tmp"

cat >> "$REPORT" <<EOF

Matching rate este calculata ca:

\`\`\`
notificari_livrate / (publicatii_trimise * subscriptii_generate) * 100
\`\`\`

Publicatiile unice livrate numara ID-urile distincte primite de cel putin un subscriber.
Notificarile livrate numara toate notificarile primite dupa deduplicarea per subscriber.
EOF

echo ""
echo "Raport generat: $REPORT"
