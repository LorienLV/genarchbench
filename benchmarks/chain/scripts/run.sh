#!/bin/bash

#PJM -N CHAIN.RUN
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

inputs_path="/fefs/scratch/bsc18/bsc18248/genarch-inputs/chain" # The folder that contains the inputs.
input_size="large" # The size of the input, "small" or "large".
compiler="gcc" # The compiler to use, "gcc" or "fcc".

if [[ $compiler != "gcc" && $compiler != "fcc" ]]; then
    echo "ERROR: The compiler must be gcc or fcc"
    exit 1
fi

if [[ $input_size != "small" && $input_size != "large" ]]; then
    echo "ERROR: The input size can only be small or large"
    exit 1
fi

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
  echo "ERROR: You have not set a valid input folder $inputs_path"
  exit 1
fi

echo "Executing..."

echo "TREADS $OMP_NUM_THREADS"

if [[ $input_size == "small" ]]; then
    ./chain_"$compiler" -i "$inputs_path"/small/in-1k.txt -o out-small.txt -t $OMP_NUM_THREADS
else
    ./chain_"$compiler" -i "$inputs_path"/large/c_elegans_40x.10k.txt -o out-large.txt -t $OMP_NUM_THREADS
fi

if [ $? -eq 0 ]; then
    echo "SUCCESS"
    exit 0
else
    echo "ERROR"
    exit 1
fi
