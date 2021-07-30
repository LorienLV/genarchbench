#!/bin/bash

#PJM -N ABEA.COMPILE
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
    echo "$compiler"
    if [[ $compiler != "gcc" && $compiler != "fcc" ]]; then
        echo "ERROR: The compiler must be gcc or fcc"
        exit 1
    fi

    if [[ $compiler == "gcc" ]]; then
        module load gcc/10.2.0
        make CC=gcc CXX=g++
    elif [[ $compiler == "fcc" ]]; then
        module load fuji
        make CC=fcc CXX=FCC
    fi
done



