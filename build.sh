#!/bin/bash
mkdir -p build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../aarch64-none-linux-gnu.toolchain.cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
make install