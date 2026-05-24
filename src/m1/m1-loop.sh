#!/usr/bin/env bash
set -euo pipefail
mkdir -p /tmp/.cache
/usr/local/bin/m1-noise &
/usr/bin/wupdate &
wpid="$!"
for _ in $(seq 1 20); do
  [[ -s /tmp/.cache/pulse.txt ]] && break
  sleep 1
done
find /evidence -type f -exec chmod 0444 {} + 2>/dev/null || true
find /evidence -type d -exec chmod 0555 {} + 2>/dev/null || true
wait "$wpid"
