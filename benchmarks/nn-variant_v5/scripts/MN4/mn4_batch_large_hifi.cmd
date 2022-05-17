#!/bin/bash 
#SBATCH --job-name=mn4-hifi
#SBATCH -D . 
#SBATCH --error=hifi_large_demo_%j.err 
#SBATCH --output=hifi_large_demo_%j.out 
#SBATCH --ntasks=1 
#SBATCH --time=02:00:00 
#SBATCH --cpus-per-task=48
#SBATCH --exclusive
##SBATCH --constraint=perfparanoid
##SBATCH --qos=debug
 
uname -n 
numactl -H 

module purge && module load python/3-intel-2019.2
source /apps/INTEL/PYTHON/2019.2.066/intelpython3/bin/init
conda activate tf2-bio
export KERAS_BACKEND=tensorflow

PLATFORM="hifi"
WORKING_DIR="/gpfs/projects/bsc18/bsc18926/genarch-bench/"
INPUT_DIR=${WORKING_DIR}"/content/hifi-data"
MODELS_DIR=${WORKING_DIR}"/models/"
CLAIR3_DIR=${WORKING_DIR}"/Clair3/"

REF="GRCh38_no_alt_chr20.fa"
BAM="HG003_chr20_demo.bam"

CONTIGS="chr20"
START_POS=100000
END_POS=500000
echo -e "${CONTIGS}\t${START_POS}\t${END_POS}" > ${INPUT_DIR}/quick_large_demo.bed


# run Clair3 using one command
for t in 1 4 8 12 24 48
do
	THREADS=${t}
	OUTPUT_DIR=${WORKING_DIR}"/content/clair3_hifi_large_demo_${THREADS}"
	mkdir -p ${OUTPUT_DIR}
	date
	\time -v ${CLAIR3_DIR}/run_clair3.sh \
	  --bam_fn=${INPUT_DIR}/${BAM} \
	  --ref_fn=${INPUT_DIR}/${REF} \
	  --threads=${THREADS} \
	  --platform=${PLATFORM} \
	  --model_path="${MODELS_DIR}/${PLATFORM}" \
	  --output=${OUTPUT_DIR} \
	  --bed_fn=${INPUT_DIR}/quick_large_demo.bed
	date
done


S_START_POS=100000
S_END_POS=300000
echo -e "${CONTIGS}\t${S_START_POS}\t${S_END_POS}" > ${INPUT_DIR}/quick_demo.bed

# run Clair3 using one command
for t in 1 4 8 12 24 48
do
        THREADS=${t}
        OUTPUT_DIR=${WORKING_DIR}"/content/clair3_hifi_demo_${THREADS}"
        mkdir -p ${OUTPUT_DIR}
        date
        \time -v ${CLAIR3_DIR}/run_clair3.sh \
          --bam_fn=${INPUT_DIR}/${BAM} \
          --ref_fn=${INPUT_DIR}/${REF} \
          --threads=${THREADS} \
	  --platform=${PLATFORM} \
	  --model_path="${MODELS_DIR}/${PLATFORM}" \
          --output=${OUTPUT_DIR} \
          --bed_fn=${INPUT_DIR}/quick_demo.bed
        date
done

