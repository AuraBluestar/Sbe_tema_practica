#!/bin/bash

set -euo pipefail

N_BROKERS=${1:-3}
N_SUBSCRIBERS=${2:-2}
N_PUBLISHERS=${3:-1}
NUM_SUBSCRIPTIONS=${4:-10}
NUM_PUBLICATIONS=${5:-10}
COMPANY_EQ_PCT=${6:-70}
PUBLISH_DELAY_MS=${7:-100}
SETTLE_SECONDS=${8:-2}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

LOG_DIR="$SCRIPT_DIR/logs/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$LOG_DIR"

PIDS=()
PUBLISHER_PIDS=()

cleanup() {
  for pid in "${PIDS[@]:-}"; do
    kill "$pid" 2>/dev/null || true
  done
}

trap cleanup EXIT

echo "=== Build ==="
bash build.sh >"$LOG_DIR/build.log" 2>&1

echo "=== Oprire procese vechi ==="
pkill -f "$SCRIPT_DIR/broker" 2>/dev/null || true
pkill -f "$SCRIPT_DIR/subscriber" 2>/dev/null || true
pkill -f "$SCRIPT_DIR/publisher" 2>/dev/null || true
pkill -f "./broker" 2>/dev/null || true
pkill -f "./subscriber" 2>/dev/null || true
pkill -f "./publisher" 2>/dev/null || true

echo "=== Pornire brokeri ==="
for id in $(seq 1 "$N_BROKERS"); do
  stdbuf -oL -eL ./broker "$id" "$N_BROKERS" >"$LOG_DIR/broker_$id.log" 2>&1 &
  PIDS+=("$!")
  echo "Broker $id -> $LOG_DIR/broker_$id.log"
done

sleep 4

echo "=== Pornire subscriberi ==="
for id in $(seq 1 "$N_SUBSCRIBERS"); do
  stdbuf -oL -eL ./subscriber "$id" "$NUM_SUBSCRIPTIONS" "$N_BROKERS" "$COMPANY_EQ_PCT" >"$LOG_DIR/subscriber_$id.log" 2>&1 &
  PIDS+=("$!")
  echo "Subscriber $id -> $LOG_DIR/subscriber_$id.log"
done

sleep "$SETTLE_SECONDS"

echo "=== Pornire publisheri ==="
for id in $(seq 1 "$N_PUBLISHERS"); do
  stdbuf -oL -eL ./publisher "$id" "$NUM_PUBLICATIONS" "$N_BROKERS" "$PUBLISH_DELAY_MS" >"$LOG_DIR/publisher_$id.log" 2>&1 &
  PIDS+=("$!")
  PUBLISHER_PIDS+=("$!")
  echo "Publisher $id -> $LOG_DIR/publisher_$id.log"
done

for pid in "${PUBLISHER_PIDS[@]}"; do
  wait "$pid" || true
done
sleep 2

echo ""
echo "=== Sumar ==="
echo "Log dir: $LOG_DIR"
grep -h "Total trimise" "$LOG_DIR"/publisher_*.log 2>/dev/null || true
grep -h "NOTIF:" "$LOG_DIR"/subscriber_*.log 2>/dev/null | tail -n 30 || true

echo ""
echo "Notificari per subscriber:"
for file in "$LOG_DIR"/subscriber_*.log; do
  [ -e "$file" ] || continue
  count=$(grep -c "NOTIF:" "$file" || true)
  echo "  $(basename "$file"): $count"
done

notification_lines=$(grep -h "NOTIF:" "$LOG_DIR"/subscriber_*.log 2>/dev/null || true)
publisher_lines=$(grep -h "Total trimise" "$LOG_DIR"/publisher_*.log 2>/dev/null || true)
total_sent=$(printf "%s\n" "$publisher_lines" | awk '{sum += $NF} END {print sum + 0}')
total_notifications=$(printf "%s\n" "$notification_lines" | grep -c "NOTIF:" || true)
unique_delivered=$({ printf "%s\n" "$notification_lines" | grep -o "id=[0-9]*" || true; } | sort -u | wc -l | awk '{print $1}')
avg_latency_us=$({ printf "%s\n" "$notification_lines" | grep -o "latenta=[0-9]*us" || true; } | sed 's/latenta=//; s/us//' | awk '{sum += $1; n++} END {if (n > 0) printf "%.2f", sum / n; else printf "0"}')

echo ""
echo "Metrici agregate:"
echo "  Publicatii trimise: $total_sent"
echo "  Publicatii unice livrate: $unique_delivered"
echo "  Notificari totale livrate: $total_notifications"
echo "  Latenta medie: $avg_latency_us us"

echo ""
echo "Forward-uri per broker:"
for file in "$LOG_DIR"/broker_*.log; do
  [ -e "$file" ] || continue
  forwards=$(grep -c "Forwarded PUBLICATION" "$file" || true)
  duplicates=$(grep -c "Duplicate PUBLICATION ignored" "$file" || true)
  echo "  $(basename "$file"): forwards=$forwards duplicates=$duplicates"
done

echo ""
echo "Rutare subscriptii per broker:"
for file in "$LOG_DIR"/broker_*.log; do
  [ -e "$file" ] || continue
  routed=$(grep -c "Routed SUBSCRIPTION" "$file" || true)
  stored_local=$(grep -c "Stored local SUBSCRIPTION" "$file" || true)
  stored_routed=$(grep -c "Stored routed SUBSCRIPTION" "$file" || true)
  complex=$(grep -c "Stored complex SUBSCRIPTION" "$file" || true)
  echo "  $(basename "$file"): routed=$routed stored_local=$stored_local stored_routed=$stored_routed complex=$complex"
done
