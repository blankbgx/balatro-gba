#!/bin/bash
# =============================================================================
# upload_rom.sh — Upload a GBAlatro ROM to Baidu cloud /gbalatro/ and
# auto-clean old ROMs every N uploads (default 10).
#
# Usage:
#   bash scripts/upload_rom.sh <local_rom_path.gba>
#
# Behavior:
#   - Counts uploads in $COUNT_FILE (default ~/.gbalatro_upload_count)
#   - On the Nth upload (UPLOAD_CLEAN_EVERY, default 10): deletes ALL .gba
#     files in /gbalatro/ first, then uploads the new ROM, resets counter to 1
#   - png assets (card art etc.) are never touched
# =============================================================================
set -u

ROM="$1"
CLEAN_EVERY="${UPLOAD_CLEAN_EVERY:-10}"
COUNT_FILE="${COUNT_FILE:-$HOME/.gbalatro_upload_count}"

# --- validate args -----------------------------------------------------------
if [ -z "$ROM" ]; then
    echo "usage: $0 <local_rom_path.gba>" >&2
    exit 1
fi
if [ ! -f "$ROM" ]; then
    echo "error: file not found: $ROM" >&2
    exit 1
fi
case "$ROM" in
    *.gba) ;;
    *) echo "error: not a .gba file: $ROM" >&2; exit 1 ;;
esac

# --- read counter ------------------------------------------------------------
COUNT=0
if [ -f "$COUNT_FILE" ]; then
    COUNT=$(cat "$COUNT_FILE" 2>/dev/null || echo 0)
fi
COUNT=$((COUNT + 1))

# --- pick a python that has bypy (hermes venv on PATH may shadow it) ---------
BYPY_PY=""
for candidate in python /c/Python313/python; do
    if $candidate -m bypy --help >/dev/null 2>&1; then
        BYPY_PY=$candidate
        break
    fi
done
[ -n "$BYPY_PY" ] || { echo "error: no python with bypy found" >&2; exit 1; }

# --- clean old ROMs when counter hits the threshold --------------------------
if [ "$COUNT" -ge "$CLEAN_EVERY" ]; then
    echo "== upload #$COUNT: cleaning old ROMs in /gbalatro/ =="
    # List remote .gba files and delete each one (tolerate transient errors)
    $BYPY_PY -m bypy list /gbalatro 2>/dev/null \
        | grep '\\.gba' \
        | awk '{print $2}' \
        | while read -r f; do
            echo "   deleting $f"
            $BYPY_PY -m bypy delete "/gbalatro/$f" >/dev/null 2>&1 || true
        done
    COUNT=1
fi

# --- upload (bypy needs relative filename + cwd, no MSYS paths) --------------
echo "== uploading $ROM ($COUNT/$CLEAN_EVERY) =="
ROM_DIR=$(cd "$(dirname "$ROM")" && pwd)
ROM_NAME=$(basename "$ROM")
(
    cd "$ROM_DIR" || exit 1
    $BYPY_PY -m bypy upload "$ROM_NAME" /gbalatro/ 2>/dev/null || {
        echo "error: upload failed" >&2
        exit 1
    }
) || exit 1

# --- persist counter ---------------------------------------------------------
echo "$COUNT" > "$COUNT_FILE"
echo "done (counter: $COUNT/$CLEAN_EVERY)"
