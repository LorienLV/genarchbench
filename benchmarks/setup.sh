#!/bin/bash

export GENARCH_BENCH_INPUTS_ROOT="PATH_TO_GENARCHBENCH/genarch-inputs"

###### OPTIONAL ######

# VTUNE
export VTUNE_HOME="$(cd "$(dirname "$(which amplxe-cl)")/../" && pwd)"
export VTUNE_INCLUDES="-I${VTUNE_HOME}/include"
export VTUNE_LDFLAGS="-L${VTUNE_HOME}/lib64 -littnotify -lpthread -ldl"

# DynamoRIO (MOD) (https://github.com/LorienLV/rapl_stopwatch)
export DYNAMORIO_INSTALL_PATH="PATH_TO_DYNAMORIO/build"

# Fujitsu PWR library
export PWR_INCLUDES="-IPATH_TO_PWR/include"
export PWR_LDFLAGS="-LPATH_TO_PWR/lib64 -lpwr"

# RAPL-Stopwatch library (https://github.com/LorienLV/rapl_stopwatch)
export RAPL_STOPWATCH_INCLUDES="-IPATH_TO_RAPL_STOPWATCH/include/rapl_stopwatch"
export RAPL_STOPWATCH_LDFLAGS="-LPATH_TO_RAPL_STOPWATCH/lib64 -lrapl_stopwatch"


