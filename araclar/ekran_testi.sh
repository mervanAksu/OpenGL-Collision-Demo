#!/bin/sh
# Programi gercek OpenGL ile, Xvfb sanal ekraninda senaryolu girdilerle calistirir.
# Yalnizca Linux: xvfb-run, Mesa (libGL.so.1) ve libX11 gerekir. GLFW/GLEW yerine
# test/ekran/sahte_glfw.c kullanilir; GLM, GLEW ve GLFW basliklari icin yollar
# EK_BAYRAK ile verilebilir. Ornek:
#   EK_BAYRAK="-I/yol/glm -I/yol/glew/include -I/yol/glfw/include" sh araclar/ekran_testi.sh
# Arguman verilmezse butun senaryolar sirayla calisir; verilirse yalnizca o dosya.
# SANITIZER=1 verilirse program AddressSanitizer + UBSan ile derlenir.
cd "$(dirname "$0")/.."
set -e
CIKTI=test/ekran/cikti
mkdir -p $CIKTI
SAN=""
[ "${SANITIZER:-0}" = "1" ] && SAN="-fsanitize=address,undefined -fno-sanitize-recover=all -g"
# ufbx x64te hiz icin maskesiz kaydirma kullanir; denetimli derlemede kendi duzeltmesi acilir
UFBX_EK=""
[ -n "$SAN" ] && UFBX_EK="-DUFBX_UBSAN"
gcc -O1 $SAN $UFBX_EK -c src/ufbx.c -o $CIKTI/ufbx.o
gcc -std=c99 -Wall -Wextra $SAN ${EK_BAYRAK:-} -DGLEW_NO_GLU -c test/ekran/sahte_glfw.c \
    -o $CIKTI/sahte_glfw.o
g++ -std=c++11 -O1 -Wall -Wextra $SAN ${EK_BAYRAK:-} -DGLEW_NO_GLU -Isrc -c src/main.cpp \
    -o $CIKTI/main.o
g++ $SAN $CIKTI/main.o $CIKTI/sahte_glfw.o $CIKTI/ufbx.o -o $CIKTI/demo -l:libGL.so.1 -lX11 -lm

# $1 senaryo, $2 istege bagli baslangic pencere boyutu (ornek 640x480)
calistir() {
    echo "== $1 ${2:-}"
    ASAN_OPTIONS=detect_leaks=0 SENARYO=$1 PENCERE_BOYUTU=$2 \
        xvfb-run -a -s "-screen 0 1280x1024x24" $CIKTI/demo
}

if [ -n "${1:-}" ]; then
    calistir "$1" "${2:-}"
    exit
fi
KALAN=0
calistir test/ekran/senaryo.txt || KALAN=1
calistir test/ekran/senaryo_erken_k.txt || KALAN=1
# senaryo.txt'nin kaydettigi 640x480 goruntusuyle karsilastirir
calistir test/ekran/senaryo_kucuk.txt 640x480 || KALAN=1
[ $KALAN = 0 ] && echo "butun ekran senaryolari gecti" || echo "EKRAN TESTI KALDI"
exit $KALAN
