#!/bin/bash

#PJM -N POA.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

# The compiler to use, "gcc" and/or "fcc".
compilers=(
    'gcc'
    'fcc'
)

module load cmake/3.19-arm

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    case "$compiler" in
        gcc)
            module load gcc/10.2.0
            make CC=gcc CXX=g++ BUILD_DIR=build_gcc arch=-march=armv8-a+sve MSA_SPOA_OMP=msa_spoa_omp_gcc
            ;;
        fcc)
            module load fuji
            make CC='fcc -Nclang' CXX='FCC -Nclang' arch=-march=armv8-a+sve BUILD_DIR=build_fcc MSA_SPOA_OMP=msa_spoa_omp_fcc
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done



