#!/bin/bash

export GENARCH_BENCH_CLUSTER="CTEARM"
export GENARCH_BENCH_INPUTS_ROOT="/fefs/scratch/bsc18/bsc18248/genarch-inputs"

export PWR_INCLUDES="-I/opt/FJSVtcs/pwrm/aarch64/include"
export PWR_LDFLAGS="-L/opt/FJSVtcs/pwrm/aarch64/lib64 -lpwr"
