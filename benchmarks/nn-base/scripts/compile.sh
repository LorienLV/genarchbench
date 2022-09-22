#!/bin/bash

# The compilers to use.
compilers=(
    'gcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            make CC=gcc CXX=g++ \
            BIN_NAME=basecall_wrapper_gcc \
            PERF_ANALYSIS=0 VTUNE_ANALYSIS=0 FAPP_ANALYSIS=0 \
            DYNAMORIO_ANALYSIS=0 RAPL_STOPWATCH=0
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
