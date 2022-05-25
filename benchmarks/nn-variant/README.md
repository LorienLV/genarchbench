# NN-Variant: variant-call benchmark 
## Based on [Clair3][Clair3]
Symphonizing pileup and full-alignment for high-performance long-read variant calling

## Installation

NN-Variant requires the following Python3 packages:

|Package|ARM|MN4|Officially Required|
|-------|--|-|-|
|Python|3.8.2|3.6.7|3.6.10|
|TF|2|2.0.0|2.1.0|
|pypy||3.6 |3.6|
|intervaltree|3.1.0|3.1.0|3.0.2|
|mpmath||1.0.0|1.2.1|
|tensorflow-addons||0.6.0|0.11.2|
|tables||3.4.4|3.6.1|
|pigz||2.4|4.4|
|parallel||20191122|20191122|
|zstd||1.0|1.4.4|
|samtools||1.10|1.1.0|
|whatshap||1.0|1.0|
|ensurepip||||

## Usage

In order to run the benchmark we need a model and input files.
Download the pre-trained models and the demos input files using the scripts:
```sh
cd scripts
source get_models.sh
source get_ont.sh
source get_hifi.sh
```

#### Benchmarks - ONT and PacBio HiFi
In the original demo script, the application have 7 stages:
1. Call variants using pileup model
2. Select heterozygous SNP variants for Whatshap phasing and haplotagging
3. Phase VCF file using Whatshap
4. Haplotag input BAM file using Whatshap
5. Select candidates for full-alignment calling
6. Call low-quality variants using full-alignment model
7. Merge pileup VCF and full-alignment VCF

##### **MareNostrum4** 
In order to run the complete demo with scalability in the number of threads, we should use the demo batch script:
```sh
cd scripts
sbatch batch_demo_ont.sh # or batch_demo_hifi.cmd
```

In general, we are more interested in the CallVar part of the application (see scalability table below), therefore, we can focus our runs with CallVar only using the following:
```sh
cd scripts
sbatch batch_ont.sh # or batch_hifi.cmd
```

If you want to modify the script and run it using interactive session, you can load the conda environment using:
```sh
module purge && module load python/3-intel-2019.2
source /apps/INTEL/PYTHON/2019.2.066/intelpython3/bin/init
conda activate tf2-bio
export KERAS_BACKEND=tensorflow
```
## Results
### MareNostrum4
The table below summarize the preformance massured using ```\time -v``` command of the Demo using ONT pre-trained model and input set.
We learn that the first stage (Call variants using pileup model) is the most time comsuming and the one that scale it's performance as we use more threads.

|Stage-ONT|1 thread|4 threads|8 threads|12 threads|24 threads|48 threads|
|-----|-|-|-|--|--|--|
|1/7|235.07|79.21|46.78|35.06|17.75|17.43|
|2/7|0.5|0.59|0.47|0.49|0.49|0.52|
|3/7|8.44|8.11|7.73|8.84|8.32|8.29|
|4/7|4.97|4.98|5.1|5|5.22|5.06|
|5/7|0.39|0.39|0.4|0.39|0.4|0.39|
|6/7|31.25|39.03|33.94|33.16|34.19|32.33|
|7/7|0.64|0.6|0.56|0.62|0.61|0.92|
|Total(Sum)|281.26|132.91|94.98|83.56|66.98|64.94|
|Total(script)|288.97|138.63|101.01|88.69|72.95|70.8|
|diff(overhead)|7.71|5.72|6.03|5.13|5.97|5.86|

Next, we run the same scalability experiment using PacBio HiFi pre-traind model and input files.
As expected, we observe similar behavior of the demo.

|Stage-HiFi|1 thread|4 threads|8 threads|12 threads|24 threads|48 threads|
|-----|-|-|-|--|--|--|
|1/7|212.4|73.12|49.03|32.9|16.27|15.9|
|2/7|0.46|0.45|0.46|0.46|0.45|0.42|
|3/7|6.59|5.62|6.66|6.77|5.58|5.4|
|4/7|2.19|2.32|2.24|2.21|2.22|2.21|
|5/7|0.37|0.36|0.36|0.36|0.36|0.36|
|6/7|18.76|18.55|17.51|16.02|17.77|19.05|
|7/7|0.55|0.63|0.53|0.55|0.53|0.59|
|Total(Sum)|241.32|101.05|76.79|59.27|43.18|43.93|
|Total(script)|248.08|105.97|82.79|65.75|48.55|49.25|
|diff(overhead)|6.76|4.92|6|6.48|5.37|5.32|

[//]: # ()

   [Clair3]: <https://github.com/HKU-BAL/Clair3>
 

