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

# TODO: bwa-mem2 does not use a build folder, so we need to execute make clean
# before compiling with a different compiler. Fix the makefile to use a build folder.

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            make CC=gcc CXX=g++ FMI=fmi_gcc arch='-mcpu=native' TARGET_ARCH=aarch64

            make TARGET_ARCH=aarch64 clean
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
