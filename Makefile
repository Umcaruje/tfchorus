
cxx = g++
flags = -std=c++11 -Wall -Werror `pkg-config lv2 --cflags` -shared -fPIC -DPIC -g
sources = src/tfcho.cpp

all: lv2 ladspa

lv2:
	mkdir -p build/tfcho.lv2
	$(cxx) $(flags) $(sources) -o build/tfcho.lv2/tfcho.so
	cp tfcho.lv2/*.ttl build/tfcho.lv2

ladspa:
	mkdir -p build/ladspa
	$(cxx) -DBUILD_LADSPA $(flags) $(sources) -o build/ladspa/tfcho.so

install:
	cp -rf build/tfcho.lv2 /usr/local/lib/lv2
	cp -rf build/ladspa/tfcho.so /usr/local/lib/ladspa

clean:
	rm -rf build/
