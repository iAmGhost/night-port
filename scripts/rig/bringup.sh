#!/usr/bin/env bash
# Bring up the dosbox-automation container for the differential rig.
#
# Pinned CPU: cycles=max makes the emulated timeline non-reproducible, so every rig run pins the
# cycle count through the API after boot (docs/100 rig plan; AGENTS.md forbids cycles=max here).
set -euo pipefail

NAME=${NAME:-rig}
IMG=${IMG:-dbga:ready}
PORT=${PORT:-8386}
WORK=/tmp/rig
CYCLES=${CYCLES:-120000}
TOKEN_FILE=$WORK/.token

mkdir -p "$WORK"
[ -f "$TOKEN_FILE" ] || { openssl rand -hex 32 > "$TOKEN_FILE"; }
TOKEN=$(cat "$TOKEN_FILE")

# the 0.85.1 linux tarball lives on the host mount (the image only carries the apt dependencies)
if ! ls -d "$WORK"/dosbox-automation-*/ >/dev/null 2>&1; then
  cp -r /tmp/dbgauto/dosbox-automation-0.85.1-7567ec4-linux-x86_64 "$WORK"/
fi

cat > "$WORK/rig.conf" <<'CONF'
[dosbox]
machine = svga_s3
memsize = 16
[cpu]
; pinned, never "max": the rig needs the same emulated time per wall-clock second in every run
core = normal
cputype = 386
cycles = fixed 120000
cycles_protected = fixed 120000
cycle_uwait = 1
cycles_mods = 0
[render]
scaler = none
[sdl]
output = texture
fullscreen = false
autolock = false
[midi]
mididevice = none
[mixer]
; 44100, not 22050: the capture endpoint writes whatever the mixer produced into a 44100 Hz WAV header, so a
; 22050 mixer yields a rate-mismatched capture (54.7% of consecutive samples identical in the old captures) and
; every sample-level or fine-spectral comparison against 44.1 kHz CD-DA was invalid (docs/130)
rate = 44100
blocksize = 256
prebuffer = 30
[serial]
serial1 = dummy
[parallel]
parallel1 = dummy
[sblaster]
sbtype = sb16
[gus]
gus = false
[joystick]
joystick_type = auto
[keyboard]
keyboard_layout = us
[webserver]
webserver_enabled = on
webserver_osd = off
; the rig talks to this over a published docker port, so it cannot stay loopback-only
webserver_bind_address = 0.0.0.0
webserver_allow_remote = true
[autoexec]
mount C /work/game
; -t cdrom, NOT -t iso: an iso-type mount exposes only the data track, so CD-DA tracks are silent in the
; emulator and the shipped-side mixer capture can never contain music (docs/129 audio_ncc NCC~0.1-0.2)
imgmount D /work/game/TG_NIGHT.CUE -t cdrom
C:
CONF
if [ "${RUN_GAME:-1}" = "1" ]; then
  printf 'NIGHT
' >> "$WORK/rig.conf"      # boot straight into the game: same timeline every run
fi

docker rm -f "$NAME" >/dev/null 2>&1 || true
docker run -d --platform linux/amd64 --name "$NAME" -v "$WORK":/work \
  -v "$HOME/Git/night-port/NIGHT:/work/game:ro" -p "$PORT:8386" \
  -e DOSBOX_API_TOKEN="$TOKEN" "$IMG" sleep 7200 >/dev/null

docker exec -d "$NAME" bash -c "cd /work/dosbox-automation-* && SDL_AUDIODRIVER=dummy \
  XVFBARGS='-screen 0 1024x768x24' xvfb-run -a -s '-screen 0 1024x768x24' \
  ./dosbox-automation -conf /work/rig.conf > /work/run.log 2>&1"

for i in $(seq 1 60); do
  if curl -s -m 3 -H "Authorization: Bearer $TOKEN" "http://localhost:$PORT/api/v1/status" >/dev/null; then
    echo "api up"; break
  fi
  sleep 1
done
curl -s -m 5 -H "Authorization: Bearer $TOKEN" "http://localhost:$PORT/api/v1/status"; echo
curl -s -m 5 -X PUT -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' \
  -d "{\"cycles\": $CYCLES}" "http://localhost:$PORT/api/v1/cpu/cycles"; echo
echo "token: $TOKEN_FILE  base: http://localhost:$PORT  cycles pinned: $CYCLES"
