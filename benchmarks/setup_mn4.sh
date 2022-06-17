#!/bin/bash

export GENARCH_BENCH_CLUSTER="MN4"
export GENARCH_BENCH_INPUTS_ROOT="/gpfs/scratch/bsc18/bsc18248/genarch-inputs"

module load vtune_amplifier/2019.4
export VTUNE_HOME="$(cd "$(dirname "$(which amplxe-cl)")/../" && pwd)"
export VTUNE_INCLUDES="-I${VTUNE_HOME}/include"
export VTUNE_LDFLAGS="-L${VTUNE_HOME}/lib64 -littnotify -lpthread -ldl"

export RAPL_STOPWATCH_INCLUDES="-I/gpfs/scratch/bsc18/bsc18248/rapl_stopwatch/install/include/rapl_stopwatch"
export RAPL_STOPWATCH_LDFLAGS="-L/gpfs/scratch/bsc18/bsc18248/rapl_stopwatch/install/lib64 -lrapl_stopwatch"
