project := "hydra"
test-file := project + "-test"
stand-file := project + "-stand"
flags := "-std=c++20 -Iinclude"
debug-flags := flags + " -g -O0"
release-flags := flags + " -O3"

alias b := build

default: test

build-test:
    mkdir -p build
    c++ test/test.cpp -o build/{{test-file}} {{debug-flags}}

build-stand:
    mkdir -p build
    c++ stand/stand.cpp -o build/{{stand-file}} {{release-flags}}

build: build-test build-stand

test: build-test
    build/{{test-file}}

stand: build-stand
    build/{{stand-file}}

clean:
    rm -rf ./build

