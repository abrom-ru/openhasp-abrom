#!/usr/bin/env bash
# Pre-render embed/montserrat.ttf into LVGL binary fonts (embed/fonts/montserratNN.bin).
# hasp_font.cpp tries L:\montserratNN.bin before the .ttf, so these replace
# FreeType rendering for the listed sizes. Requires node (npx lv_font_conv).
#
#   bash tools/gen_montserrat_fonts.sh

set -euo pipefail
cd "$(dirname "$0")/.."

TTF="embed/montserrat.ttf"
OUT="embed/fonts"
CONV="npx -y lv_font_conv@1.5.2"

# Latin + Latin-1 symbols (°, «», ±), Cyrillic with Ёё, dashes, quotes, bullet, ellipsis, №
SMALL_RANGE="0x20-0x7E,0xA0-0xBF,0x401,0x410-0x44F,0x451,0x2013-0x2014,0x2018-0x201E,0x2022,0x2026,0x2116"
SMALL_SIZES="12 14 16 18 20 24"

# Clock / temperature only: space % + , - . / 0-9 : C F ° –
BIG_RANGE="0x20,0x25,0x2B-0x3A,0x43,0x46,0xB0,0x2013"
BIG_SIZES="56 128"

mkdir -p "$OUT"

gen() { # size range
    $CONV --font "$TTF" -r "$2" --size "$1" --bpp 4 --format bin --no-kerning -o "$OUT/montserrat$1.bin"
    echo "montserrat$1.bin  $(wc -c < "$OUT/montserrat$1.bin") bytes"
}

for s in $SMALL_SIZES; do gen "$s" "$SMALL_RANGE"; done
for s in $BIG_SIZES; do gen "$s" "$BIG_RANGE"; done

echo "total: $(cat "$OUT"/*.bin | wc -c) bytes"
