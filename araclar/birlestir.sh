#!/bin/sh
# parcalar/*.inc dosyalarindan src/main.cpp uretir.
# Yalnizca "#ifndef TEST_DERLEME" bloklarinin kendi satirlari atilir (icerikleri kalir);
# diger #if/#ifdef bloklari ve onlarin #endif'leri oldugu gibi korunur.
cd "$(dirname "$0")/.."
set -e
GECICI=$(mktemp)
trap 'rm -f "$GECICI"' EXIT
{
    cat parcalar/baslik.inc parcalar/ortak.inc
    # sabitler parcalarda daginik durur, tek dosyada basta toplanir
    for p in fbx_model carpisma kamera sahne; do
        grep -E '^const (int|float) ' parcalar/$p.inc
    done
    for p in fbx_model carpisma kamera sahne; do
        echo
        awk '
            function hata(m) {
                print "birlestir: " FILENAME ":" NR ": " m > "/dev/stderr"
                bozuk = 1
                exit 1
            }
            /^[ \t]*#[ \t]*ifndef[ \t]+TEST_DERLEME[ \t]*$/ { yigin[++d] = "at"; next }
            /^[ \t]*#[ \t]*if/ { yigin[++d] = "tut"; print; next }
            /^[ \t]*#[ \t]*(else|elif)/ {
                if (d == 0) hata("acilmamis blokta #else")
                if (yigin[d] == "at") hata("TEST_DERLEME blogunda #else olamaz")
                print; next
            }
            /^[ \t]*#[ \t]*endif/ {
                if (d == 0) hata("fazladan #endif")
                if (yigin[d--] == "at") next
                print; next
            }
            /^const (int|float) / { next }
            { print }
            END { if (!bozuk && d != 0) hata("kapanmamis #if blogu") }
        ' parcalar/$p.inc || exit 1
    done
} > "$GECICI"
cat -s "$GECICI" > src/main.cpp
echo "src/main.cpp: $(wc -l < src/main.cpp) satir"
