#!/bin/sh
# Keymap guard rails for the macropad layout. Fails (exit 1) on:
#   1. any transparent key (&trans) anywhere in the keymap  -- nothing may fall through
#      to the default layer underneath
#   2. any layer that does not have exactly 42 bindings
#   3. a layer/output/system key on the GAME layer other than the &game_exit hold-to-exit
#
# Comments are stripped first, so prose may mention these words freely.
# POSIX sh + awk, no other dependencies:  sh scripts/lint-keymap.sh [path/to/file.keymap]

KEYMAP="${1:-config/corne.keymap}"
KEYS_PER_LAYER=42
fail=0

[ -f "$KEYMAP" ] || { echo "lint: $KEYMAP not found" >&2; exit 2; }

# drop /* ... */ and // ... comments (block comments may span lines)
stripped=$(awk '
  { line = $0; out = ""
    while (length(line) > 0) {
      if (inb) {
        i = index(line, "*/")
        if (i == 0) { line = "" } else { line = substr(line, i + 2); inb = 0 }
      } else {
        i = index(line, "/*"); j = index(line, "//")
        if (j > 0 && (i == 0 || j < i)) { out = out substr(line, 1, j - 1); line = "" }
        else if (i > 0) { out = out substr(line, 1, i - 1); line = substr(line, i + 2); inb = 1 }
        else { out = out line; line = "" }
      }
    }
    print out }' "$KEYMAP")

# 1. no &trans
if printf '%s\n' "$stripped" | grep -n '&trans'; then
  echo "lint: FAIL: &trans found (use &none or an explicit key)" >&2
  fail=1
fi

# 2. every layer has exactly KEYS_PER_LAYER bindings
counts=$(printf '%s\n' "$stripped" | tr '\n' ' ' | awk -v want="$KEYS_PER_LAYER" '
  { s = $0; k = index(s, "\"zmk,keymap\""); if (k == 0) { print "NOKEYMAP"; exit }
    s = substr(s, k); n = 0
    while ((i = index(s, "display-name")) > 0) {
      s = substr(s, i)
      nm = s; sub(/^display-name *= *"/, "", nm); sub(/".*/, "", nm)
      j = index(s, "bindings"); s = substr(s, j); j = index(s, "<"); s = substr(s, j + 1)
      e = index(s, ">;"); body = substr(s, 1, e - 1); c = gsub(/&/, "&", body)
      printf "%s %d\n", nm, c; s = substr(s, e)
    } }')
if [ "$counts" = "NOKEYMAP" ] || [ -z "$counts" ]; then
  echo "lint: FAIL: no zmk,keymap layers found" >&2; fail=1
else
  echo "$counts" | while read -r name c; do
    if [ "$c" -ne "$KEYS_PER_LAYER" ]; then
      echo "lint: FAIL: layer $name has $c bindings, expected $KEYS_PER_LAYER" >&2
      exit 1
    fi
  done || fail=1
fi

# 3. GAME layer has no layer/output/system keys except &game_exit
game=$(printf '%s\n' "$stripped" | tr '\n' ' ' | awk '
  { i = index($0, "game_layer {"); if (i == 0) exit
    s = substr($0, i); e = index(s, "};"); print substr(s, 1, e) }')
if [ -n "$game" ]; then
  if printf '%s\n' "$game" | grep -Eq '&(mo|to|tog|sl|lt|sk|bt|out|bootloader|sys_reset|rgb_ug|soft_off|studio_unlock)([^A-Za-z0-9_]|$)'; then
    echo "lint: FAIL: GAME layer contains a layer/output/system key (only &game_exit is allowed)" >&2
    fail=1
  fi
else
  echo "lint: FAIL: game_layer not found" >&2; fail=1
fi

if [ "$fail" -eq 0 ]; then echo "lint: ok ($KEYMAP)"; fi
exit "$fail"
