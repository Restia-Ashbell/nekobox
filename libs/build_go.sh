#!/bin/bash
set -e

source libs/env_deploy.sh
[ "$GOOS" == "windows" ] && [ "$GOARCH" == "amd64" ] && DEST=$DEPLOYMENT/windows-amd64 || true
[ "$GOOS" == "windows" ] && [ "$GOARCH" == "arm64" ] && DEST=$DEPLOYMENT/windows-arm64 || true
[ "$GOOS" == "linux" ] && [ "$GOARCH" == "amd64" ] && DEST=$DEPLOYMENT/linux-amd64 || true
[ "$GOOS" == "linux" ] && [ "$GOARCH" == "arm64" ] && DEST=$DEPLOYMENT/linux-arm64 || true
[ "$GOOS" == "darwin" ] && [ "$GOARCH" == "amd64" ] && DEST=$DEPLOYMENT/macos-amd64 || true
[ "$GOOS" == "darwin" ] && [ "$GOARCH" == "arm64" ] && DEST=$DEPLOYMENT/macos-arm64 || true

if [ -z $DEST ]; then
  echo "Please set GOOS GOARCH"
  exit 1
fi
rm -rf $DEST
mkdir -p $DEST

#### Go: updater ####
# pushd go/cmd/updater
# [ "$GOOS" == "darwin" ] || go build -o $DEST -trimpath -ldflags "-w -s"
# [ "$GOOS" == "linux" ] && mv $DEST/updater $DEST/launcher || true
# popd

#### Go: nekobox_core ####
go build -v -buildmode=c-shared -trimpath -ldflags "-w -s -X github.com/sagernet/sing-box/constant.Version=$(git rev-parse --short HEAD)" -tags "with_clash_api,with_gvisor,with_quic,with_wireguard,with_utls,with_dhcp" -o ./build/ ./cmd/sing-box