export OMP_PROC_BIND=true
source "/fefs/scratch/bsc18/bsc18248/genarch-bench/benchmarks/nn-variant-v5/scripts/../../setup_ctearm.sh"
module load fuji/1.2.26b tensorflow/2.1.0-dnnl
source /apps/TENSORFLOW/2.1.0-dnnl/FUJI/VENV/fccbuild_v210/bin/activate
module load parallel samtools
/fefs/scratch/bsc18/bsc18248/genarch-bench/benchmarks/nn-variant_v5/Clair3/callVar.sh --bam_fn="/fefs/scratch/bsc18/bsc18248/genarch-inputs/nn-variant/HG003_chr20_demo.bam" --ref_fn="/fefs/scratch/bsc18/bsc18248/genarch-inputs/nn-variant/GRCh38_no_alt_chr20.fa" --threads=1 --platform=ont --model_path="/fefs/scratch/bsc18/bsc18248/genarch-bench/benchmarks/nn-variant_v5/models/r941_prom_hac_g360+g422" --bed_fn=region.bed --output=. --chunk_size=32222084