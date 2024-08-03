.PHONY: all debug release ut

all: release

init:
	cmake --preset=default

clean:
	rm -rf build

debug:
	cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug
	cmake --build build

release:
	cd build && cmake .. -DCMAKE_BUILD_TYPE=Release
	cmake --build build

ut:debug
	./build/mergekv