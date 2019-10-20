#!/bin/sh

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

export AFLSMART=$SCRIPTPATH
export PATH=$PATH:$AFLSMART:$AFLSMART/peach-3.0.202-source/output/linux_x86_64_debug/bin
export AFL_PATH=$AFLSMART
export LD_LIBRARY_PATH=/usr/local/lib

