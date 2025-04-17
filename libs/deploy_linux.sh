#!/bin/bash
set -e

if [[ $(uname -m) == 'aarch64' || $(uname -m) == 'arm64' ]]; then
  ARCH="arm64"
else
  ARCH="amd64"
fi

source libs/env_deploy.sh
DEST=$DEPLOYMENT/linux-$ARCH
rm -rf $DEST
mkdir -p $DEST

#### copy binary ####
cp $BUILD/nekoray $DEST

cd download-artifact
cd *linux-$ARCH
tar xvzf artifacts.tgz -C ../../
cd ..
cd *public_res
tar xvzf artifacts.tgz -C ../../
cd ../..

mv $DEPLOYMENT/public_res/* $DEST
rmdir $DEPLOYMENT/public_res

sudo add-apt-repository universe
sudo apt install libfuse2
if [ "$ARCH" = "arm64" ]; then
  ARCH="aarch64"
elif [ "$ARCH" = "amd64" ]; then
  ARCH="x86_64"
fi
wget https://github.com/linuxdeploy/linuxdeploy/releases/latest/download/linuxdeploy-$ARCH.AppImage -O linuxdeploy.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/latest/download/linuxdeploy-plugin-qt-$ARCH.AppImage -O linuxdeploy-plugin-qt.AppImage
chmod +x linuxdeploy.AppImage linuxdeploy-plugin-qt.AppImage

export EXTRA_QT_PLUGINS="svg;iconengines;"
./linuxdeploy.AppImage --appdir $DEST --executable $DEST/nekoray --plugin qt
rm linuxdeploy.AppImage linuxdeploy-plugin-qt.AppImage
cd $DEST
rm nekoray
mv ./usr/bin/nekoray .
rm -r ./usr/translations ./usr/bin ./usr/share ./apprun-hooks