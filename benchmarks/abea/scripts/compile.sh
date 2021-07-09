#!/bin/bash

#PJM -N ABEA.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

compiler="fcc" # The compiler to use, "gcc" or "fcc".

if [[ $compiler != "gcc" && $compiler != "fcc" ]]; then
    echo "ERROR: The compiler must be gcc or fcc"
    exit 1
fi

if [[ $compiler == "gcc" ]]; then
    module load gcc/10.2.0
    make -f Makefile.gcc
elif [[ $compiler == "fcc" ]]; then
    module load fuji
    # To prevent this error: Catastrophic error: could not set locale "" to allow processing of multibyte characters
    export LANG=en_US.utf8
    export LC_ALL=en_US.utf8
    make -f Makefile.fcc
fi

if [ $? -eq 0 ]; then
    echo "SUCCESS"
    exit 0
else
    echo "ERROR"
    exit 1
fi
