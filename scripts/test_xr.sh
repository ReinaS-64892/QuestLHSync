#!/bin/sh
# Requires a C++17 compiler, pkg-config and SDL2 development files. No HMD/runtime needed.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests
${CXX:-c++} -O1 -g -std=c++17 -Wall -Wno-unused-function -pthread \
  -Ithird_party -Ithird_party/openxr -Ithird_party/stb $(pkg-config --cflags sdl2) \
  tests/xr_tracking_test.cpp src/driver/sync.cpp src/driver/net.cpp \
  $(pkg-config --libs sdl2) -ldl -o build/tests/xr_tracking_test
build/tests/xr_tracking_test
