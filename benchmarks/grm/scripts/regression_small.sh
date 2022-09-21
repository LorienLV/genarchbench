#!/bin/bash

# The folder that contains the inputs for this app.
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/grm/small"

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
    echo "ERROR: You have not set a valid input folder $inputs_path"
    exit 1
fi

scriptfolder="$(dirname $(realpath $0))"
binaries_path="$(dirname "$scriptfolder")/2.0"

# Clean the stage folder of the jobs after finishing? 1 -> yes, 0 -> no.
clean=1

# The name of the job.
job="GRM-REGRESSION-SMALL"

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

# Nodes, MPI ranks and OMP theads used to execute with each command.
# parallelism=(
#     'nodes=1, mpi=1, omp=1'
#     'nodes=1, mpi=1, omp=2'
#     'nodes=1, mpi=1, omp=4'
#     'nodes=1, mpi=1, omp=8'
#     'nodes=1, mpi=1, omp=12'
#     'nodes=1, mpi=1, omp=24'
#     'nodes=1, mpi=1, omp=36'
#     'nodes=1, mpi=1, omp=48'
# )

# # job additional parameters.
# job_options=(
#     # '--exclusive'
#     # '--time=00:00:01'
#     # '--qos=debug'
# )

# Everything you want to do before executing the commands.
before_command="export OMP_PROC_BIND=true; export OMP_PLACES=cores;"

case "$GENARCH_BENCH_CLUSTER" in
MN4)
    before_command="module load intel/2020.1; module load mkl/2021.3;"

    commands=(
        "module load gcc/10.1.0_binutils; $binaries_path/build_dynamic_gcc/plink2"
    )

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

    job_options=(
        '--exclusive'
        '--time=00:00:40'
    )
    ;;
CTEARM)
    before_command+="source $scriptfolder/../../setup_ctearm.sh;"

    commands=(
        "module load gcc/10.2.0; $binaries_path/build_dynamic_gcc/plink2"
        "module load fuji; $binaries_path/build_dynamic_fcc/plink2"
    )

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

    job_options=(
        '-L rscgrp=large'
    )
    ;;
*)
    commands=(
        "$binaries_path/build_dynamic_gcc/plink2"
    )

    parallelism=(
        'nodes=1, mpi=1, omp=1'
        'nodes=1, mpi=1, omp=2'
        'nodes=1, mpi=1, omp=4'
    )
    ;;
esac

# Additional arguments to pass to the commands.
command_opts="--maf 0.01 --pgen \"$inputs_path/chr22_phase3.pgen\" \
--pvar \"$inputs_path/chr22_phase3.pvar\" \
--psam \"$inputs_path/phase3_corrected.psam\" \
--make-grm-bin --out out --threads \$OMP_NUM_THREADS"

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

    # Check if the output files are identical to the reference
    # diff --brief out.grm.bin "$inputs_path/reference.grm.bin"
    # one=$?
    one=0
    diff --brief out.grm.id "$inputs_path/reference.grm.id"
    two=$?
    diff --brief out.grm.N.bin "$inputs_path/reference.grm.N.bin"
    three=$?

    if [[ $one -ne 0 || $two -ne 0 || $three -ne 0 ]]; then
        echo "The output file is not identical to the reference file"
        return 1 # Failure
    fi

    # Elapsed time.
    cat $job_name.err | grep "Time in GRM:"

    return 0 # OK
)

source "$scriptfolder/../../run_wrapper.sh"
