#!/bin/bash

set -euo pipefail

BUILD_TYPE="Debug"
BUILD_DIR="code/build"
REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
LOCAL_PICO_SDK="$REPO_ROOT/code/pico-sdk"
LOCAL_TOOLCHAIN_BIN="$REPO_ROOT/tools/arm-gnu-toolchain/bin"
LOCAL_PICOTOOL_DIR="$REPO_ROOT/tools/picotool"
EXTRA_CMAKE_ARGS=()

if [ -d "$LOCAL_PICO_SDK" ] && [ -z "${PICO_SDK_PATH:-}" ]; then
  export PICO_SDK_PATH="$LOCAL_PICO_SDK"
  echo "Using local Pico SDK: $PICO_SDK_PATH"
fi

if [ -d "$LOCAL_TOOLCHAIN_BIN" ] && [ -z "${PICO_TOOLCHAIN_PATH:-}" ]; then
  export PICO_TOOLCHAIN_PATH="$LOCAL_TOOLCHAIN_BIN"
  echo "Using local ARM toolchain: $PICO_TOOLCHAIN_PATH"
fi

if [ -d "$LOCAL_PICOTOOL_DIR" ]; then
  EXTRA_CMAKE_ARGS+=("-Dpicotool_DIR=$LOCAL_PICOTOOL_DIR")
fi

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" "${EXTRA_CMAKE_ARGS[@]}" ..
