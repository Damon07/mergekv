.PHONY: all debug release ut

all: release

clean:
	rm -rf build

debug:
	mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug
	cmake --build build

release:
	mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release
	cmake --build build

ut:debug
	./build/mergekv