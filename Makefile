CXX = g++
CC = gcc
HEDEF = collision_demo

ifeq ($(OS),Windows_NT)
    LIBS = -lglfw3 -lglew32 -lopengl32 -lgdi32 -static-libgcc -static-libstdc++
    HEDEF := $(HEDEF).exe
else ifeq ($(shell uname),Darwin)
    LIBS = -lglfw -lGLEW -framework OpenGL
    CXXFLAGS += -DGL_SILENCE_DEPRECATION -I/opt/homebrew/include -I/usr/local/include
    LDFLAGS += -L/opt/homebrew/lib -L/usr/local/lib
else
    LIBS = -lglfw -lGLEW -lGL -lm
endif

CXXFLAGS += -std=c++11 -O2 -Wall

$(HEDEF): src/main.o src/ufbx.o
	$(CXX) $^ -o $@ $(LDFLAGS) $(LIBS)

src/main.o: src/main.cpp src/ufbx.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/ufbx.o: src/ufbx.c src/ufbx.h
	$(CC) -O2 -c $< -o $@

.PHONY: calistir temizle

calistir: $(HEDEF)
	./$(HEDEF)

temizle:
	rm -f src/*.o $(HEDEF)
