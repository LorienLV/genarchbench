#!/bin/bash

export GENARCH_BENCH_CLUSTER="CTEARM"
export GENARCH_BENCH_INPUTS_ROOT="/fefs/scratch/bsc18/bsc18248/genarch-inputs"

export DYNAMORIO_INSTALL_PATH="/fefs/scratch/bsc18/bsc18248/dynamorio/build"
export DYNAMORIO_OS="LINUX"
export DYNAMORIO_ARCH="ARM_64"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${DYNAMORIO_INSTALL_PATH}/lib64/release"
export DYNAMORIO_INCLUDES="-D${DYNAMORIO_OS} -D${DYNAMORIO_ARCH} -I${DYNAMORIO_INSTALL_PATH}/include"
export DYNAMORIO_LDFLAGS="-L${DYNAMORIO_INSTALL_PATH}/lib64/release -ldynamorio"