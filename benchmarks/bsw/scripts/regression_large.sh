#!/bin/bash

# The folder that contains the inputs for this app.
inputs_path="/fefs/scratch/bsc18/bsc18248/genarch-inputs/bsw/large"

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
  echo "ERROR: You have not set a valid input folder $inputs_path"
  exit 1
fi

scriptfolder="$(dirname $(realpath $0))"
binaries_path="$(dirname "$scriptfolder")"

# Clean the stage folder of the jobs after finishing? 1 -> yes, 0 -> no.
clean=1

# The name of the job.
job="BSW-REGRESSION-LARGE"

# job additional parameters.
job_options=(
    # '--exclusive'
    # '--time=00:00:01'
    # '--qos=debug'
)

# Commands to run.
# You can access the number of mpi-ranks using the environment variable
# MPI_RANKS and the number of omp-threads using the environment variable
# OMP_NUM_THREADS:
# commands=(
#    'command $MPI_RANKS $OMP_NUM_THREADS'
#)
# OR
# commands=(
#    "command \$MPI_RANKS \$OMP_NUM_THREADS"
#)
commands=(
    "module load gcc/10.2.0; $binaries_path/build_gcc/main_bsw"
    "module load fuji; $binaries_path/build_fcc/main_bsw"
)

# Additional arguments to pass to the commands.
command_opts="-pairs \"$inputs_path/bandedSWA_SRR7733443_1m_input.txt\" -t \$OMP_NUM_THREADS -b 512"

# Nodes, MPI ranks and OMP theads used to execute with each command.
parallelism=(
    'nodes=1, mpi=1, omp=1'
    'nodes=1, mpi=1, omp=2'
    'nodes=1, mpi=1, omp=4'
    'nodes=1, mpi=1, omp=8'
    'nodes=1, mpi=1, omp=12'
    'nodes=1, mpi=1, omp=24'
    'nodes=1, mpi=1, omp=36'
    'nodes=1, mpi=1, omp=48'
)

#
# Additional variables.
#

#
# This function is executed before launching a job. You can use this function to
# prepare the stage folder of the job.
#
before_run() (
    job_name="$1"
)

#
# This function is executed when a job has finished. You can use this function to
# perform a sanity check and to output something in the report of a job.
#
# echo: The message that you want to output in the report of the job.
# return: 0 if the run is correct, 1 otherwise.
#
after_run() (
    job_name="$1"

    pairs_processed="$(cat "$job_name.out" | grep "Total Pairs processed" | cut -d ' ' -f 4)"
    kernel_time="$(cat "$job_name.out" | grep "Overall SW cycles" | cut -d ' ' -f 6)"

    if [[ $pairs_processed -ne 10606460 ]]; then
        echo "Error in the execution $pairs_processed"
        return 1 # Failure
    fi

    echo "Kernel execution time $kernel_time s"

    return 0 # OK
)

source "$scriptfolder/../../regression.sh"
