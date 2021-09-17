# anded Smith-Waterman (BSW)

`bsw` uses the same license as [BWA-MEM2](https://github.com/bwa-mem2/bwa-mem2).

If you use `bsw`, please cite:

```
@inproceedings{DBLP:conf/ipps/VasimuddinMLA19,
  author    = {Md. Vasimuddin and
               Sanchit Misra and
               Heng Li and
               Srinivas Aluru},
  title     = {Efficient Architecture-Aware Acceleration of {BWA-MEM} for Multicore
               Systems},
  booktitle = {2019 {IEEE} International Parallel and Distributed Processing Symposium,
               {IPDPS} 2019, Rio de Janeiro, Brazil, May 20-24, 2019},
  pages     = {314--324},
  publisher = {{IEEE}},
  year      = {2019},
  url       = {https://doi.org/10.1109/IPDPS.2019.00041},
  doi       = {10.1109/IPDPS.2019.00041},
  timestamp = {Wed, 16 Oct 2019 14:14:51 +0200},
  biburl    = {https://dblp.org/rec/conf/ipps/VasimuddinMLA19.bib},
  bibsource = {dblp computer science bibliography, https://dblp.org}
}
```

## Compilation

To compile BSW in CTE-ARM you can use [scripts/compile_ctearm.sh](scripts/compile_ctearm.sh). By default the script compiles BSW with GCC and FCC. You can easily add new compilers or compiler versions by modifying the script. To execute the script:

```
pjsub --interact
bash scripts/compile_ctearm.sh

OR

pjsub scripts/compile_ctearm.sh
```

Alternatively, you can directly use the provided Makefile. The benchmark is architecture-dependent; it supports aarch64 and x86_64. To manually compile, you should pass the target architecture and architecture specific flags when executing make: `make arch=native TARGET_ARCH=x86_64`. If `TARGET_ARCH` is not provided, the Makefile will automatically detect the machine's architecture using `arch` Linux command.

## Execution

```
./main_bsw -pairs <pairs_file>  -t <num_threads> -b <batch_size>
```

## Run regression tests

1. Before executing the regression tests, you must set the environment variable `GENARCH_BENCH_INPUTS_ROOT` to the root location of the inputs; the directory that contains a folder with the name of each benchmark. If you are executing in CTE-ARM, you can use [setup_ctearm.sh](../setup_ctearm.sh) to automatically prepare the environment for executing in the cluster.

2. There are two regression tests provided: [scripts/regression_small.sh](scripts/regression_small.sh), that executes a small test-case, and [scripts/regression_large.sh](scripts/regression_large.sh), that executes a large test-case. Each test executes the corresponding test-case using the GCC and FCC versions of BSW with different combinations of OMP threads. The scripts also print the execution time of each benchmark. To execute the tests:

    ```
    bash scripts/regression_small.sh
    bash scripts/regression_large.sh
    ```
