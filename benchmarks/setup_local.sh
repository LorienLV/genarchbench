#!/bin/bash

export GENARCH_BENCH_CLUSTER="local"
export GENARCH_BENCH_INPUTS_ROOT="/home/lorien/Repos/genomicsbench/inputs/genarch-inputs"

export VTUNE_HOME="$(cd "$(dirname "$(which vtune)")/../" && pwd)"
export VTUNE_INCLUDES="-I${VTUNE_HOME}/include"
export VTUNE_LDFLAGS="-L${VTUNE_HOME}/lib64 -littnotify -lpthread -ldl"

export DYNAMORIO_INSTALL_PATH="/home/lorien/Repos/dynamorio/build"
