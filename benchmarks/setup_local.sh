#!/bin/bash

export GENARCH_BENCH_CLUSTER="local"
export GENARCH_BENCH_INPUTS_ROOT="/home/lorien/Repos/genomicsbench/inputs/genarch-inputs"

export VTUNE_HOME="$(cd "$(dirname "$(which vtune)")/../" && pwd)"
