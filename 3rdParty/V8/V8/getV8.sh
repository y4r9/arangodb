#!/bin/bash
#exit on errors
set -euo pipefail
V8VersionToGet="$1"
V8PATH="$2"
if [ -z "$V8VersionToGet" ]; then
    exit 1
fi
if [ ! -d "$V8PATH" ]; then
    exit 1
fi
if [ ! -d "$V8PATH/depot_tools" ]; then
    exit 1
fi
export PATH="$V8PATH/depot_tools":$PATH
if [ ! -d "$V8PATH/v8/v8" ]; then
    cd "$V8PATH/v8"
    fetch v8
    cd v8
    git checkout "branch-heads/$V8VersionToGet"
    gclient update
    #only has to be run once
    #the following requires shell access to aquire root privileges
    #unfortunately the directories have to be wrapped for everything to work,
    #since google checks for .git stuff seemingly
    ./build/install-build-deps.sh
else
    cd "$V8PATH/v8/v8"
    git checkout "branch-heads/$V8VersionToGet"
    gclient sync
fi
