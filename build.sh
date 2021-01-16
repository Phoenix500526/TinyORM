#!/bin/sh
set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-./build}
BUILD_TYPE=${BUILD_TYPE:-debug}

mkdir -p $BUILD_DIR \
  && cd $BUILD_DIR \
  && conan install .. \
            --build missing     \
            -s compiler.libcxx=libstdc++11  \
  && cmake .. \
           -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  && make $*