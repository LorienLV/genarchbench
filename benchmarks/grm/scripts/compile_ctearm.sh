#!/bin/bash

#PJM -N CHAIN.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

# The compiler to use, "gcc" and/or "fcc".
compilers=(
    'gcc'
    # 'fcc'
)

# TODO: bwa-mem2 does not use a build folder, so we need to execute make clean
# before compiling with a different compiler. Fix the makefile to use a build folder.

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    build_dir="2.0/build_dynamic_$compiler"
    mkdir "$build_dir"
    cp -r "2.0/build_dynamic/." "$build_dir"

    pushd "$build_dir"

    case "$compiler" in
        gcc)
            module load gcc/10.2.0
            # module load arm/20.3

            make CC=gcc CXX=g++ arch='-march=armv8-a+sve'
            ;;
        fcc)
            module load fuji

            make CC='fcc -Nclang' CXX='FCC -Nclang' arch='-march=armv8-a+sve'
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac

    popd
done
