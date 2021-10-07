#!/bin/bash

#PJM -N ABEA.CONFIG
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

# The folder that contains the inputs.
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/abea"

scriptfolder="$(realpath $0)"
binaries_path="$(dirname "$(dirname "$(realpath $0)")")"
binary="$binaries_path/f5c_gcc" # The ABEA binary to use.

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
    echo "ERROR: You have not set a valid input folder $inputs_path"
    exit 1
fi

echo "Creating the index files..."

"$binary" index -d "$inputs_path"/fast5_files "$inputs_path"/1000reads.fastq
"$binary" index -d "$inputs_path"/fast5_files "$inputs_path"/10000reads.fastq
