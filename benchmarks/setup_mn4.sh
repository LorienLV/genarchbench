#!/bin/bash

export GENARCH_BENCH_CLUSTER="MN4"
export GENARCH_BENCH_INPUTS_ROOT="/gpfs/scratch/bsc18/bsc18248/genarch-inputs"

module load vtune_amplifier/2019.4
export VTUNE_HOME="$(cd "$(dirname "$(which amplxe-cl)")/../" && pwd)"

module load intel/2019.4
module load parallel_studio/2019.4
export ADVISOR_HOME="$(cd "$(dirname "$(which advixe-cl)")/../" && pwd)"
