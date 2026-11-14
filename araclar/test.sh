#!/bin/sh
# OpenGL gerektirmeyen birim testleri (carpisma matematigi, frustum, FBX okuma)
cd "$(dirname "$0")/.."
set -e
g++ -std=c++11 -Wall -DTEST_DERLEME test/carpisma_test.cpp -o test/carpisma_test
./test/carpisma_test | tail -1
g++ -std=c++11 -Wall test/frustum_test.cpp -o test/frustum_test
./test/frustum_test | tail -1
