#!/bin/bash

# Execute "source ../setup_*.sh" before running this script.
# Load the required modules before executing this script.

# The folder that contains the inputs.
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/abea"

scriptfolder="$(dirname $(realpath $0))"
binaries_path="$(dirname "$scriptfolder")"

binary="$binaries_path/f5c_gcc" # The ABEA binary to use.

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
    echo "ERROR: You have not set a valid input folder $inputs_path"
    exit 1
fi

echo "Creating the index files..."

"$binary" index -d "$inputs_path"/fast5_files "$inputs_path"/1000reads.fastq
"$binary" index -d "$inputs_path"/fast5_files "$inputs_path"/10000reads.fastq
