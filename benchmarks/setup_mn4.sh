#!/bin/bash

export GENARCH_BENCH_CLUSTER="MN4"
export GENARCH_BENCH_INPUTS_ROOT="/gpfs/scratch/bsc18/bsc18248/genarch-inputs"

module load vtune_amplifier/2019.4
export VTUNE_HOME="$(cd "$(dirname "$(which amplxe-cl)")/../" && pwd)"
