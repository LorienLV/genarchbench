# Adaptive Banded Signal to Event Alignment (ABEA)

`abea` uses the same license as [f5c](https://github.com/hasindu2008/f5c).

If you find `abea` useful, please cite:

```
@article{gamaarachchi2020gpu,
  title={GPU accelerated adaptive banded event alignment for rapid comparative nanopore signal analysis},
  author={Gamaarachchi, Hasindu and Lam, Chun Wai and Jayatilaka, Gihan and Samarakoon, Hiruna and Simpson, Jared T and Smith, Martin A and Parameswaran, Sri},
  journal={BMC bioinformatics},
  volume={21},
  number={1},
  pages={1--13},
  year={2020},
  publisher={BioMed Central}
}
```

## Compilation

To compile ABEA in CTE-ARM you can use [scripts/compile_ctearm.sh](scripts/compile_ctearm.sh). By default the script compiles ABEA with GCC and FCC. You can easily add new compilers or compiler versions by modifying the script. To execute the script:

```
pjsub --interact
bash scripts/compile.sh

OR

pjsub scripts/compile.sh
```

Alternatively, you can directly use the provided Makefile.

## Execution

```
./f5c index -d <fast5-files-folder> <fastq reads> # To generate the index files.
./f5c eventalign -b <bam> -g <ref genome fasta> -r <fastq reads> # To execute.
```

## Run regression tests

1. Before executing the regression tests, you must set the environment variable `GENARCH_BENCH_INPUTS_ROOT` to the root location of the inputs; the directory that contains a folder with the name of each benchmark. If you are executing in CTE-ARM, you can use [setup_ctearm.sh](../setup_ctearm.sh) to automatically prepare the environment for executing in the cluster.

2. If you move the input files to another folder you will have to regenerate the index files. To do so, you can execute [scrtips/generate_indexes.sh](scripts/generate_indexes.sh):

    ```
    pjsub --interact
    bash scripts/generate_indexes.sh

    OR

    pjsub scripts/generate_indexes.sh
    ```

By default, the script uses the GCC ABEA version, so you will need to compile that version before trying to generate the index files.

3. There are two regression tests provided: [scripts/regression_small.sh](scripts/regression_small.sh), that executes a small test-case, and [scripts/regression_large.sh](scripts/regression_large.sh), that executes a large test-case. Each test executes the corresponding test-case using the GCC and FCC versions of ABEA. The tests check that the results are correct by comparing the obtained output with the result obtained on an Intel machine. The scripts also print the execution time of each benchmark. To execute the tests:

    ```
    bash scripts/regression_small.sh
    bash scripts/regression_large.sh
    ```