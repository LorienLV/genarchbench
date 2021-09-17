# Seed Chaining (CHAIN)

`chain` uses the same license as [Minimap2](https://github.com/lh3/minimap2).

If you find `chain` useful, please cite:

```
@article{li2018minimap2,
  title={Minimap2: pairwise alignment for nucleotide sequences},
  author={Li, Heng},
  journal={Bioinformatics},
  volume={34},
  number={18},
  pages={3094--3100},
  year={2018},
  publisher={Oxford University Press}
}
```

## Compilation

To compile CHAIN in CTE-ARM you can use [scripts/compile_ctearm.sh](scripts/compile_ctearm.sh). By default the script compiles CHAIN with GCC and FCC. You can easily add new compilers or compiler versions by modifying the script. To execute the script:

```
pjsub --interact
bash scripts/compile_ctearm.sh

OR

pjsub scripts/compile_ctearm.sh
```

Alternatively, you can directly use the provided Makefile.

## Execution

```
./chain -i <input_file> -o <output_file> -t <num_threads>
```

## Run regression tests

1. Before executing the regression tests, you must set the environment variable `GENARCH_BENCH_INPUTS_ROOT` to the root location of the inputs; the directory that contains a folder with the name of each benchmark. If you are executing in CTE-ARM, you can use [setup_ctearm.sh](../setup_ctearm.sh) to automatically prepare the environment for executing in the cluster.

2. There are two regression tests provided: [scripts/regression_small.sh](scripts/regression_small.sh), that executes a small test-case, and [scripts/regression_large.sh](scripts/regression_large.sh), that executes a large test-case. Each test executes the corresponding test-case using the GCC and FCC versions of CHAIN with different combinations of OMP threads. The tests check that the results are correct by comparing the obtained output with the results obtained on an Intel machine. The scripts also print the execution time of each benchmark. To execute the tests:

    ```
    bash scripts/regression_small.sh
    bash scripts/regression_large.sh
    ```
