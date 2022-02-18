#!/bin/bash

# The compilers to use.
compilers=(
    'gcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    build_dir="2.0/build_dynamic_$compiler"
    mkdir "$build_dir"
    cp -r "2.0/build_dynamic/." "$build_dir"

    pushd "$build_dir"

    case "$compiler" in
        gcc)
            make CC=gcc CXX=g++ arch=-march=native \
            DYNAMORIO_ANALYSIS=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac

    popd
done
