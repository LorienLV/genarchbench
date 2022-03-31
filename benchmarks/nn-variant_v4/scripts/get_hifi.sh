PLATFORM="hifi"
INPUT_DIR="content/hifi-data"
mkdir -p ${INPUT_DIR}
OUTPUT_DIR="content/clair3_hifi_demo"
mkdir -p ${OUTPUT_DIR}

# download quick demo data
# GRCh38_no_alt Reference
wget --quiet -P ${INPUT_DIR} http://www.bio8.cs.hku.hk/clair3/demo/quick_demo/pacbio_hifi/GRCh38_no_alt_chr20.fa
wget --quiet -P ${INPUT_DIR} http://www.bio8.cs.hku.hk/clair3/demo/quick_demo/pacbio_hifi/GRCh38_no_alt_chr20.fa.fai
# BAM chr20:100000-300000
wget --quiet -P ${INPUT_DIR} http://www.bio8.cs.hku.hk/clair3/demo/quick_demo/pacbio_hifi/HG003_chr20_demo.bam
wget --quiet -P ${INPUT_DIR} http://www.bio8.cs.hku.hk/clair3/demo/quick_demo/pacbio_hifi/HG003_chr20_demo.bam.bai
# GIAB Truth VCF and BED
wget --quiet -P ${INPUT_DIR} http://www.bio8.cs.hku.hk/clair3/demo/quick_demo/pacbio_hifi/HG003_GRCh38_chr20_v4.2.1_benchmark.vcf.gz
wget --quiet -P ${INPUT_DIR} http://www.bio8.cs.hku.hk/clair3/demo/quick_demo/pacbio_hifi/HG003_GRCh38_chr20_v4.2.1_benchmark.vcf.gz.tbi
wget --quiet -P ${INPUT_DIR} http://www.bio8.cs.hku.hk/clair3/demo/quick_demo/pacbio_hifi/HG003_GRCh38_chr20_v4.2.1_benchmark_noinconsistent.bed

REF="GRCh38_no_alt_chr20.fa"
BAM="HG003_chr20_demo.bam"

CONTIGS="chr20"
START_POS=100000
END_POS=300000
echo -e "${CONTIGS}\t${START_POS}\t${END_POS}" > ${INPUT_DIR}/quick_demo.bed

