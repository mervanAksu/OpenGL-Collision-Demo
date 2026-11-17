#!/bin/sh
# OpenGL gerektirmeyen birim testleri; proje kokunden calisir (models/ okunur).
# MSYS2, Linux ve macOS'ta GLM/GLEW/GLFW basliklari varsayilan yoldaysa ek ayar gerekmez.
# Degilse: EK_BAYRAK="-I/yol/glm -I/yol/glew/include -I/yol/glfw/include" sh araclar/test.sh
# SANITIZER=1 ile AddressSanitizer + UBSan altinda calisir (MinGW'de yok, Linux/macOS).
cd "$(dirname "$0")/.."
set -e
CIKTI=test/cikti
mkdir -p $CIKTI
SAN=""
[ "${SANITIZER:-0}" = "1" ] && SAN="-fsanitize=address,undefined -fno-sanitize-recover=all -g"
B="-std=c++11 -O1 -Wall -Wextra $SAN ${EK_BAYRAK:-} -DTEST_DERLEME -DGLEW_NO_GLU -Isrc -Iparcalar"
# ufbx x64te hiz icin maskesiz kaydirma kullanir; denetimli derlemede kendi duzeltmesi acilir
UFBX_EK=""
[ -n "$SAN" ] && UFBX_EK="-DUFBX_UBSAN"
gcc -O1 $SAN $UFBX_EK -c src/ufbx.c -o $CIKTI/ufbx.o
g++ $B test/carpisma_test.cpp -o $CIKTI/carpisma_test
g++ $B test/frustum_test.cpp -o $CIKTI/frustum_test
g++ $B test/fbx_yukle_test.cpp $CIKTI/ufbx.o -lm -o $CIKTI/fbx_yukle_test
g++ $B test/sahne_test.cpp $CIKTI/ufbx.o -lm -o $CIKTI/sahne_test
for t in carpisma_test frustum_test fbx_yukle_test sahne_test; do
    printf '%-16s ' "$t:"
    if ! ./$CIKTI/$t > $CIKTI/$t.log 2>&1; then
        echo "KALDI"
        grep -E "HATA|KALDI|runtime error|ERROR" $CIKTI/$t.log | head -20
        exit 1
    fi
    tail -1 $CIKTI/$t.log
done
