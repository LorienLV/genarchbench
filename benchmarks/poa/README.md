# Partial-Order Alignment (POA)

`poa` uses the same license as [SIMD partial order alignment library](https://github.com/rvaser/spoa).

If you find `poa` useful, please cite:

```
@article{vaser2017fast,
  title={Fast and accurate de novo genome assembly from long uncorrected reads},
  author={Vaser, Robert and Sovi{\'c}, Ivan and Nagarajan, Niranjan and {\v{S}}iki{\'c}, Mile},
  journal={Genome research},
  volume={27},
  number={5},
  pages={737--746},
  year={2017},
  publisher={Cold Spring Harbor Lab}
}
```

## Compilation

To compile POA in CTE-ARM you can use [scripts/compile_ctearm.sh](scripts/compile_ctearm.sh). By default the script compiles POA with GCC and FCC. You can easily add new compilers or compiler versions by modifying the script. To execute the script:

```
pjsub --interact
bash scripts/compile_ctearm.sh

OR

pjsub scripts/compile_ctearm.sh
```

Alternatively, you can directly use the provided Makefile.

## Execution

```
./msa_spoa -s <input_file> -t <num_threads> > out.fasta
```

## Run regression tests

1. Before executing the regression tests, you must set the environment variable `GENARCH_BENCH_INPUTS_ROOT` to the root location of the inputs; the directory that contains a folder with the name of each benchmark. If you are executing in CTE-ARM, you can use [setup_ctearm.sh](../setup_ctearm.sh) to automatically prepare the environment for executing in the cluster.

2. There are two regression tests provided: [scripts/regression_small.sh](scripts/regression_small.sh), that executes a small test-case, and [scripts/regression_large.sh](scripts/regression_large.sh), that executes a large test-case. Each test executes the corresponding test-case using the GCC and FCC versions of POA with different combinations of OMP threads. The tests check that the results are correct by comparing the obtained output with the results obtained on an Intel machine. The scripts also print the execution time of each benchmark. To execute the tests:

    ```
    bash scripts/regression_small.sh
    bash scripts/regression_large.sh
    ```

