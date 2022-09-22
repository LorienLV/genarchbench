#!/bin/bash

# The folder that contains the inputs.
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/nn-variant"

if [[ -z "$inputs_path" || ! -d "$inputs_path" ]]; then
    echo "ERROR: You have not set a valid input folder $inputs_path"
    exit 1
fi

scriptfolder="$(dirname $(realpath $0))"
benchmark_path="$(dirname "$scriptfolder")"

# Clean the stage folder of the jobs after finishing? 1 -> yes, 0 -> no.
clean=1

# The name of the job.
job="NN-VARIANT-REGRESSION-SMALL"

contig="chr20"
contig_len=64444167 # Mbps
start_pos=100000
end_pos=200000

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

# job additional parameters.
# job_options=(
#     '--exclusive'
#     '--time=00:03:00'
# )

# Everything you want to do before executing the commands.
before_command="export OMP_PROC_BIND=true; export OMP_PLACES=cores;"

commands=(
    "$benchmark_path/variantcaller_wrapper_gcc"
)

parallelism=(
    'nodes=1, mpi=1, omp=1'
    'nodes=1, mpi=1, omp=2'
    'nodes=1, mpi=1, omp=4'
)

# Additional arguments to pass to the commands.
command_opts="\"$benchmark_path/Clair3/callVar.sh\" \
	          --bam_fn=\"$inputs_path/HG002_GRCh38_ONT-UL_GIAB_20200122_chr20_0_10000000.phased.bam\" \
	          --ref_fn=\"$inputs_path/hg38_chr20.fa\" \
	          --threads=\$OMP_NUM_THREADS \
	          --platform="ont" \
	          --model_path=\"$benchmark_path/models/r941_prom_hac_g360+g422\" \
	          --bed_fn=region.bed \
              --output=. \
              --chunk_size=\$((${contig_len}/\${OMP_NUM_THREADS}+1)) \
              \$PYPY"

#
# This function is executed before launching a job. You can use this function to
# prepare the stage folder of the job.
#
before_run() (
    job_name="$1"

    echo -e "${contig}\t${start_pos}\t${end_pos}" > region.bed
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

    diff --brief \
        <(cat "./tmp/pileup_output/"*.vcf | grep -v "^#" | sort -u -k 2,2 -n | cut -f 1-5,7-9) \
        <(cat "$inputs_path/HG002_chr20_100000_200000_reference.vcf" | grep -v "^#" | sort -k 2,2 -n | cut -f 1-5,7-9)

    if [[ $? -ne 0 ]]; then
        echo "The output file is not identical to the reference file"
        return 1 # Failure
    fi

    cat "$job_name.err" | grep "Energy consumption:"
    cat "$job_name.err" | grep "VariantCalling execution time:"

    return 0 # OK
)

source "$scriptfolder/../../run_wrapper.sh"
