#!/bin/bash

export GENARCH_BENCH_CLUSTER="CTEAMD"
export GENARCH_BENCH_INPUTS_ROOT="/gpfs/scratch/bsc18/bsc18248/genarch-inputs"

# module load vtune_amplifier/2019.4
# export VTUNE_HOME="$(cd "$(dirname "$(which amplxe-cl)")/../" && pwd)"
# export VTUNE_INCLUDES="-I${VTUNE_HOME}/include"
# export VTUNE_LDFLAGS="-L${VTUNE_HOME}/lib64 -littnotify -lpthread -ldl"

# export DYNAMORIO_INSTALL_PATH="/gpfs/scratch/bsc18/bsc18248/dynamorio/build"

export RAPL_STOPWATCH_INCLUDES="-I/gpfs/scratch/bsc18/bsc18248/CTEAMD/rapl_stopwatch/install/include/rapl_stopwatch"
export RAPL_STOPWATCH_LDFLAGS="-L/gpfs/scratch/bsc18/bsc18248/CTEAMD/rapl_stopwatch/install/lib64 -lrapl_stopwatch"
