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
            make CC=gcc CXX=g++ FOLDER_BUILD=build_gcc FOLDER_BIN=bin_gcc \
            VTUNE_ANALYSIS=1 DYNAMORIO_ANALYSIS=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
