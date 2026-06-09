#!/bin/bash

N_BROKERS=$1
N_SUBS=$2
N_PUBS=$3

num_subscriptions=10
num_publications=10
BASE_PORT=5001

if [ -z "$N_BROKERS" ] || [ -z "$N_SUBS" ] || [ -z "$N_PUBS" ]; then
  echo "Usage: ./run_system.sh <brokers> <subs> <pubs>"
  exit 1
fi

pkill -f "./broker" 2>/dev/null
pkill -f "./subscriber" 2>/dev/null
pkill -f "./publisher" 2>/dev/null
