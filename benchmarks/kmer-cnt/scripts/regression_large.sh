#!/bin/bash

# The folder that contains the inputs for this app.
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/kmer-cnt/large"

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
    echo "ERROR: You have not set a valid input folder $inputs_path"
    exit 1
fi

scriptfolder="$(dirname $(realpath $0))"
configfolder="$(dirname "$scriptfolder")/config"
binaries_path="$(dirname "$scriptfolder")"

# Clean the stage folder of the jobs after finishing? 1 -> yes, 0 -> no.
clean=1

# The name of the job.
job="KMERCNT-REGRESSION-LARGE"

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

# job additional parameters.
# job_options=(
#     # '--exclusive'
#     # '--time=00:00:01'
#     # '--qos=debug'
# )

# Everything you want to do before executing the commands.
before_command="export OMP_PROC_BIND=true; export OMP_PLACES=cores;"

case "$GENARCH_BENCH_CLUSTER" in
MN4)
    commands=(
        "module load gcc/10.1.0; $binaries_path/kmer-cnt_gcc"
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
        '--time=00:07:00'
        '--constraint=highmem'
    )
    ;;
CTEAMD)
    commands=(
        "module load gcc/10.2.0; $binaries_path/kmer-cnt_gcc"
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
        '--time=00:07:00'
    )
    ;;
CTEARM)
    before_command+="source $scriptfolder/../../setup_ctearm.sh;"

    commands=(
        "module load gcc/10.2.0; $binaries_path/kmer-cnt_gcc"
        "module load fuji; $binaries_path/kmer-cnt_fcc"
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
GR3)
    commands=(
        "$binaries_path/kmer-cnt_gcc"
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
    ;;
*)
    commands=(
        "$binaries_path/kmer-cnt_gcc"
    )

    parallelism=(
        'nodes=1, mpi=1, omp=1'
        'nodes=1, mpi=1, omp=2'
        'nodes=1, mpi=1, omp=4'
    )
    ;;
esac

# Additional arguments to pass to the commands.
command_opts="--reads \"$inputs_path/Loman_E.coli_MAP006-1_2D_50x.fasta\" \
              --config \"$configfolder/asm_raw_reads.cfg\" --debug --threads \$OMP_NUM_THREADS"

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

    refkmers="$(cat "$inputs_path/output-reference.txt" | grep -m 1 -Eo "Total k-mers [0-9]+" | cut -d ' ' -f 3)"
    outkmers="$(cat "$job_name.err" | grep -m 1 -Eo "Total k-mers [0-9]+" | cut -d ' ' -f 3)"

    wall_time="$(cat "$job_name.err" | grep -m 1 "Kernel time:" | cut -d ' ' -f 3)"

    if [[ -z "$wall_time" ]]; then
        echo "Error in the execution"
        return 1 # Failure
    fi
    # if [[ "$refkmers" != "$outkmers" ]]; then
    #     echo "Reference number of k-mers != output number of k-mers"
    #     return 1 # Failure
    # fi

    echo "Kernel time: $wall_time s"
    cat "$job_name.err" | grep "Energy consumption:"

    return 0 # OK
)

source "$scriptfolder/../../run_wrapper.sh"
