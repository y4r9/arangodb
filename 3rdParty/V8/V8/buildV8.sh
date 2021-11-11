#!/bin/bash
#exit on errors
set -euo pipefail
V8PATH="$1"
if [ ! -d "$V8PATH" ]; then
    exit 1
fi
if [ ! -d "$V8PATH/depot_tools" ]; then
    exit 1
fi
if [ ! -d "$V8PATH/v8" ]; then
    exit 1
fi
if [ ! -d "$V8PATH/v8/v8" ]; then
    exit 1
fi
if [ ! -f "$V8PATH/v8/buildConfig.txt" ]; then
    exit 1
fi
export PATH="$V8PATH/depot_tools":$PATH
BUILD_PARAMS=$(cat "$V8PATH/v8/buildConfig.txt")
BUILD_PARAMS="${BUILD_PARAMS//;/ }"
DestinationPath="$2"
BUILD_ARCH="$3"
if [ -z "$BUILD_ARCH" ]; then
    exit 1
fi
case "$BUILD_ARCH" in
    ia32|x64|arm|arm64|mipsel|mips64el|ppc|ppc64|riscv64|s390|s390x|android_arm|android_arm64|loong64) ;;
    *) exit 1;;
esac
BUILD_MODE="$4"
if [ -z "$BUILD_MODE" ]; then
    exit 1
fi
case "$BUILD_MODE" in
    Release|Debug|Optdebug) ;;
    *) exit 1;;
esac

if [ ! -d "$DestinationPath" ]; then
    exit 1
fi

cd "$V8PATH/v8/v8"
COMMAND="gn gen \"${DestinationPath}\" --args=\"${BUILD_PARAMS}\""
#TODO:
#FIXME: Evil parameters may escape to shell
eval $COMMAND
gn clean "${DestinationPath}"
gn gen --check "${DestinationPath}"
autoninja -C "${DestinationPath}" v8_monolith
