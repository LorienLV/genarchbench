#!/bin/bash

#PJM -N ABEA.CONFIG
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

inputs_path="/home/lorien/Repos/genomicsbench/inputs/genarch-inputs/abea" # The folder that contains the inputs.
compiler="gcc" # The compiler to use, "gcc" or "fcc".

if [[ $compiler != "gcc" && $compiler != "fcc" ]]; then
    echo "ERROR: The compiler must be gcc or fcc"
    exit 1
fi

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
  echo "ERROR: You have not set a valid input folder $inputs_path"
  exit 1
fi

echo "Creating the index files..."
# Small
./f5c_"$compiler" index -d "$inputs_path"/fast5_files "$inputs_path"/1000reads.fastq
# Large
./f5c_"$compiler" index -d "$inputs_path"/fast5_files "$inputs_path"/10000reads.fastq

