#!/bin/bash
set -e
pushd $(dirname $0) &> /dev/null

CMAKE_FLAGS=(
  "-DCMAKE_BUILD_TYPE=Debug"
  "-DCMAKE_C_FLAGS=-std=gnu99"
  "-DCMAKE_EXPORT_COMPILE_COMMANDS=1"
)

function __build() {
  [ -d build ] || {
    mkdir build
  }
  pushd build &> /dev/null
    cmake .. "${CMAKE_FLAGS[@]}" "$@"
    make -j$(nproc --all)
  popd &> /dev/null
}

function __clean() {
  [ -d build ] && {
    rm -r build
  }
}

case $1 in
  build) { __build "${@:2}"; };;
  clean) { __clean;          };;
  *) {
    echo "unknown arguments: $@" >& 2
    echo "try: "
    echo "  build"
    echo "  clean"
  };;
esac

popd &> /dev/null