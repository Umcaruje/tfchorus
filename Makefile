
cxx = g++
flags = -std=c++11 -Wall -Werror `pkg-config lv2 --cflags` -shared -fPIC -DPIC -g
sources = src/tfcho.cpp

all: lv2

lv2:
	mkdir -p build/tfcho.lv2
	$(cxx) $(flags) $(sources) -o build/tfcho.lv2/tfcho.so
	cp tfcho.lv2/*.ttl build/tfcho.lv2

install:
	cp -rf build/tfcho.lv2 ~/.lv2
