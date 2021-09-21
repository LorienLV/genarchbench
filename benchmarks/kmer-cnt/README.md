# K-mer Counting (KMER-CNT)

`kmer-cnt` uses the same license as [Flye](https://github.com/fenderglass/Flye).

If you find `kmer-cnt` useful, please cite:

```
@article{kolmogorov2019assembly,
  title={Assembly of long, error-prone reads using repeat graphs},
  author={Kolmogorov, Mikhail and Yuan, Jeffrey and Lin, Yu and Pevzner, Pavel A},
  journal={Nature biotechnology},
  volume={37},
  number={5},
  pages={540--546},
  year={2019},
  publisher={Nature Publishing Group}
}
```

## Compilation

To compile KMER-CNT in CTE-ARM you can use [scripts/compile_ctearm.sh](scripts/compile_ctearm.sh). By default the script compiles KMER-CNT with GCC and FCC. You can easily add new compilers or compiler versions by modifying the script. To execute the script:

```
pjsub --interact
bash scripts/compile_ctearm.sh

OR

pjsub scripts/compile_ctearm.sh
```

Alternatively, you can directly use the provided Makefile.

## Execution

```
./kmer-cnt --reads <input_file> --config <config_file> --threads <num_threads> [--debug]
```

## Run regression tests

1. Before executing the regression tests, you must set the environment variable `GENARCH_BENCH_INPUTS_ROOT` to the root location of the inputs; the directory that contains a folder with the name of each benchmark. If you are executing in CTE-ARM, you can use [setup_ctearm.sh](../setup_ctearm.sh) to automatically prepare the environment for executing in the cluster.

2. There are two regression tests provided: [scripts/regression_small.sh](scripts/regression_small.sh), that executes a small test-case, and [scripts/regression_large.sh](scripts/regression_large.sh), that executes a large test-case. Each test executes the corresponding test-case using the GCC and FCC versions of KMER-CNT with different combinations of OMP threads. The tests check that the results are correct by comparing the obtained output with the results obtained on an Intel machine. The scripts also print the execution time of each benchmark. To execute the tests:

    ```
    bash scripts/regression_small.sh
    bash scripts/regression_large.sh
    ```


