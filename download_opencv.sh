#!/bin/bash
if [[ ! -d 3rdparty/libopencv-4.5.5-aarch64 ]]; then
    cd 3rdparty
    wget https://github.com/ZHEQIUSHUI/assets/releases/download/ax650/libopencv-4.5.5-aarch64.zip
    unzip libopencv-4.5.5-aarch64.zip
    cd ..
fi