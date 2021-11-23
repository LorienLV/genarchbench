#!/bin/bash

if [[ -z "$GENARCH_BENCH_CLUSTER" ]]; then
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
            make CC=gcc CXX=g++ arch=native BUILD_PATH=build_gcc BIN_NAME=chain_gcc \
            # VTUNE_ANALYSIS=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
