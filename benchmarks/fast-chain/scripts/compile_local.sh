#!/bin/bash

# The compilers to use.
compilers=(
    'gcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            make CC=gcc CXX=g++ arch=native BUILD_PATH=build_gcc BIN_NAME=chain_gcc \
            DYNAMORIO_ANALYSIS=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
