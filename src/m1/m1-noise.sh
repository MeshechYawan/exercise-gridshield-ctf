#!/usr/bin/env bash
set -euo pipefail
while true; do
  getent hosts cbfs01.gridshield.local >/dev/null 2>&1 || true
  getent hosts novasec.gridshield.local >/dev/null 2>&1 || true
  curl -s --connect-timeout 2 http://198.51.100.20/ >/dev/null 2>&1 || true
  nc -z -w1 198.51.100.20 80 >/dev/null 2>&1 || true
  sleep $((5 + RANDOM % 7))
done
