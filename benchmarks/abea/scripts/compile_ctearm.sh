#!/bin/bash

#PJM -N ABEA.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

if [[ "$GENARCH_BENCH_CLUSTER" != "CTEARM" ]]; then
    echo "ERROR: Run 'source setup_ctearm.sh' before using this script"
    exit 1
fi

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
            make CC=gcc CXX=g++ BINARY=f5c_gcc BUILD_DIR=build_gcc
            ;;
        fcc)
            module load fuji
            # To prevent this error: Catastrophic error: could not set locale "" to 
            # allow processing of multibyte characters
            export LANG=en_US.utf8
            export LC_ALL=en_US.utf8
            make CC='fcc -Nclang' CXX='FCC -Nclang' BINARY=f5c_fcc BUILD_DIR=build_fcc \
            FAPP_ANALYSIS=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac
done
