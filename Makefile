
install:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

build:
	cmake --build build


.PHONY: install build clean
