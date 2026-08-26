.PHONY: all run clean
all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build -j
run: all
	./build/day04_rtos http://localhost:8080
clean:
	cmake -E remove_directory build
