

install:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

build: install
	cmake --build build

server: build
	./build/Server

client: build
	./build/Client

test: build
	./build/Test

coverage: test
	mkdir -p build/coverage
	$(MAKE) -C build/ coverage
	open build/coverage/coverage.html

.PHONY: install build clean coverage
