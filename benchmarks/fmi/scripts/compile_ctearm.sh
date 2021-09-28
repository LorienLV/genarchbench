#!/bin/bash

#PJM -N CHAIN.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

# The compiler to use, "gcc" and/or "fcc".
compilers=(
    'gcc'
    'fcc'
)

# TODO: bwa-mem2 does not use a build folder, so we need to execute make clean
# before compiling with a different compiler. Fix the makefile to use a build folder.

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            module load gcc/10.2.0
            make CC=gcc CXX=g++ FMI=fmi_gcc arch=-march=armv8-a+sve TARGET_ARCH=aarch64

            make TARGET_ARCH=aarch64 clean
            ;;
        fcc)
            module load fuji
            make CC=fcc CXX='FCC -Nclang -lstdc++' FMI=fmi_fcc arch=-march=armv8-a+sve TARGET_ARCH=aarch64

            make TARGET_ARCH=aarch64 clean
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
