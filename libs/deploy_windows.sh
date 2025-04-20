#!/bin/bash
set -e

source libs/env_deploy.sh
DEST=$DEPLOYMENT/windows-$ARCH
rm -rf $DEST
mkdir -p $DEST

#### copy exe ####
cp $BUILD/nekoray.exe $DEST

cd download-artifact
cd *windows-$ARCH
tar xvzf artifacts.tgz -C ../../
cd ..
cd *public_res
tar xvzf artifacts.tgz -C ../../
cd ../..

mv $DEPLOYMENT/public_res/* $DEST
rmdir $DEPLOYMENT/public_res

#### deploy qt & DLL runtime ####
pushd $DEST
windeployqt nekoray.exe --no-translations --no-system-d3d-compiler --no-system-dxc-compiler --no-compiler-runtime --no-opengl-sw -force-openssl --verbose 2
cp C:/Windows/System32/msvcp140.dll        ./
cp C:/Windows/System32/msvcp140_1.dll      ./
cp C:/Windows/System32/vcruntime140.dll    ./
cp C:/Windows/System32/vcruntime140_1.dll  ./
popd
