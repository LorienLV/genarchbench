#!/bin/bash

export GENARCH_BENCH_CLUSTER="local"
export GENARCH_BENCH_INPUTS_ROOT="/home/lorien/Repos/genomicsbench/inputs/genarch-inputs"

export VTUNE_HOME="$(cd "$(dirname "$(which vtune)")/../" && pwd)"
export VTUNE_INCLUDES="-I${VTUNE_HOME}/include"
export VTUNE_LDFLAGS="-L${VTUNE_HOME}/lib64 -littnotify -lpthread -ldl"

export DYNAMORIO_INSTALL_PATH="/home/lorien/Repos/dynamorio/build"
export DYNAMORIO_OS="LINUX"
export DYNAMORIO_ARCH="X86_64"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${DYNAMORIO_INSTALL_PATH}/lib64/release"
export DYNAMORIO_INCLUDES="-D${DYNAMORIO_OS} -D${DYNAMORIO_ARCH} -I${DYNAMORIO_INSTALL_PATH}/include"
export DYNAMORIO_LDFLAGS="-L${DYNAMORIO_INSTALL_PATH}/lib64/release -ldynamorio"
