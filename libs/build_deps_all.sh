#!/bin/bash
set -e

cd libs

if [ -z $deps ]; then
  deps="deps"
fi

# libs/deps/...
mkdir -p $deps
cd $deps
if [ -z $NKR_PACKAGE ]; then
  INSTALL_PREFIX=$PWD/built
else
  INSTALL_PREFIX=$PWD/package
fi
rm -rf $INSTALL_PREFIX
mkdir -p $INSTALL_PREFIX

#### clean ####
clean() {
  rm -rf dl.zip yaml-* zxing-* protobuf
}

#### protobuf ####
git clone -b v31.1 --depth 1 https://github.com/protocolbuffers/protobuf

#备注：交叉编译要在 host 也安装 protobuf 并且版本一致,编译安装，同参数，安装到 /usr/local

cd protobuf

cmake -S . -B build -GNinja -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -Dprotobuf_MSVC_STATIC_RUNTIME=OFF -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX
cmake --build build --target install

cd ..

####
clean
