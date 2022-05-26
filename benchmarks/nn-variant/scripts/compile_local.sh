#!/bin/bash

if [[ "$GENARCH_BENCH_CLUSTER" != "local" ]]; then
    echo "ERROR: Run 'source setup_local.sh' before using this script"
    exit 1
fi

# The compilers to use.
compilers=(
    'gcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            make CC=gcc CXX=g++ \
            BIN_NAME=variantcaller_wrapper_gcc \
            VTUNE_ANALYSIS=1 DYNAMORIO_ANALYSIS=0
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done