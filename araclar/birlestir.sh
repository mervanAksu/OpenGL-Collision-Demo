#!/bin/sh
# parcalar/*.inc dosyalarindan src/main.cpp uretir (test korumalari ayiklanir)
cd "$(dirname "$0")/.."
{
    cat parcalar/baslik.inc parcalar/ortak.inc
    # sabitler parcalarda daginik durur, tek dosyada basta toplanir
    for p in fbx_model carpisma kamera sahne; do
        grep -E '^const (int|float) ' parcalar/$p.inc
    done
    for p in fbx_model carpisma kamera sahne; do
        echo
        grep -v -E -e '^#ifndef TEST_DERLEME' -e '^#endif' -e '^const (int|float) ' parcalar/$p.inc
    done | cat -s
} > src/main.cpp
echo "src/main.cpp: $(wc -l < src/main.cpp) satir"
