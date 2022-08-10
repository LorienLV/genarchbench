#!/bin/bash

if [[ "$GENARCH_BENCH_CLUSTER" != "GR3" ]]; then
    echo "ERROR: Run 'source setup_gr3.sh' before using this script"
    exit 1
fi

export OMP_NUM_THREADS=1

# The compiler to use.
compilers=(
    'gcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            make CC=gcc CXX=g++ arch='-mcpu=native' \
            BUILD_PATH=build_gcc BIN_NAME=chain_gcc
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
