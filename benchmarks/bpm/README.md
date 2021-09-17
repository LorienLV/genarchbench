# Bit-Parallel Myers (BPM)

## Compilation

To compile BPM in CTE-ARM you can use [scripts/compile_ctearm.sh](scripts/compile_ctearm.sh). By default the script compiles BPM with GCC and FCC. You can easily add new compilers or compiler versions by modifying the script. To execute the script:

```
pjsub --interact
bash scripts/compile_ctearm.sh

OR

pjsub scripts/compile_ctearm.sh
```

Alternatively, you can directly use the provided Makefile.

## Generate datasets

```
./bin/generate_datasets -n 1000000 -l 100 -e 0.05 -o input.n1M.l100.seq
./bin/generate_datasets -n 100000 -l 1000 -e 0.05 -o input.n100K.l1K.seq
```

## Run benchmark

```
./bin/align_benchmark -a bpm-edit -i input.n1M.l100.seq
./bin/align_benchmark -a bpm-edit -i input.n100K.l1K.seq
```

## Generate checksum

Use option '-o'  to generate a dump of the results

```
./bin/align_benchmark -a bpm-edit -i input.n1M.l100.seq  -o checksum.file
./bin/align_benchmark -a bpm-edit -i input.n100K.l1K.seq -o checksum.file
```

## Run regression tests

1. Before executing the regression tests, you must set the environment variable `GENARCH_BENCH_INPUTS_ROOT` to the root location of the inputs; the directory that contains a folder with the name of each benchmark. If you are executing in CTE-ARM, you can use [setup_ctearm.sh](../setup_ctearm.sh) to automatically prepare the environment for executing in the cluster.

2. There are two regression tests provided: [scripts/regression_small.sh](scripts/regression_small.sh), that executes a small test-case, and [scripts/regression_large.sh](scripts/regression_large.sh), that executes a large test-case. Each test executes the corresponding test-case using the GCC and FCC versions of BPM. The tests check that the results are correct by comparing the obtained checksums with the results obtained on an Intel machine. The scripts also print the execution time of each benchmark. To execute the tests:

    ```
    bash scripts/regression_small.sh
    bash scripts/regression_large.sh
    ```
