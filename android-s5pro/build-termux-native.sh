#!/data/data/com.termux/files/usr/bin/bash
# build-termux-native.sh - build libs5dng.so (arm64) on-device via Termux
set -e
cd "$(dirname "$0")"

[ -d build-termux ] || cmake -S . -B build-termux -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DS5CONV_ANDROID=OFF \
  -DCMAKE_PREFIX_PATH=$PREFIX
# NOTE: on-device we build the CLI first (validates toolchain), then the JNI lib
cmake --build build-termux --target s5core -j$(nproc)
# JNI lib needs android/log.h which Termux lacks; provide a shim header path:
mkdir -p build-termux/android-shim
[ -f build-termux/android-shim/log.h ] || cat > build-termux/android-shim/log.h <<'EOF'
#ifndef ANDROID_LOG_H_SHIM
#define ANDROID_LOG_H_SHIM
#include <stdio.h>
#define __android_log_print(prio, tag, ...) do { fprintf(stderr, "%s: ", tag); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define ANDROID_LOG_INFO  4
#define ANDROID_LOG_ERROR 6
#endif
EOF
cmake -S . -B build-termux -DCMAKE_CXX_FLAGS="-I$PWD/build-termux/android-shim" >/dev/null
cmake --build build-termux --target s5dng -j$(nproc)

echo "native lib: $(ls build-termux/libs5dng.so)"
