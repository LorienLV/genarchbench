#!/bin/bash

#PJM -N BPM.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

# The compiler to use, "gcc" and/or "fcc".
compilers=(
    'gcc'
    'fcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            module load gcc/10.2.0
            make CC=gcc CXX=g++ arch=-march=armv8-a+sve FOLDER_BUILD=build_gcc FOLDER_BIN=bin_gcc
            ;;
        fcc)
            module load fuji
            make CC='fcc -Nclang' CXX='FCC -Nclang' arch=-march=armv8-a+sve FOLDER_BUILD=build_fcc FOLDER_BIN=bin_fcc
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
