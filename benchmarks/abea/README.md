# ABEA

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

## Compile in AFX64:

To compile ABEA you can use [scripts/compile.sh](scripts/compile.sh). By default the script compiles ABEA with GCC and FCC. To execute the script:

```
pjsub --interact
bash scripts/compile.sh
```
OR
```
pjsub scripts/compile.sh
```

Alternatively, you can load the required modules and execute `make CC=gcc CXX=g++` to compile with GCC and `make CC=fcc CXX=FCC` to compile with FCC.

## Run regression tests

1. The inputs and the reference outputs of ABEA are located in `/fefs/scratch/bsc18/bsc18248/genarch-inputs/abea` in CTE-ARM.

2. If you move the input files to another folder you will have to regenerate the index files. To do so, first set the variable `inputs_path` to the new path in [scripts/generate_indexes.sh](scripts/generate_indexes.sh), and then execute the script:

```
pjsub --interact
bash scripts/generate_indexes.sh
```
OR
```
pjsub scripts/generate_indexes.sh
```

By default, the script uses the GCC ABEA version, so you will need to compile that version before trying to generate the index files.


3. There are two regression tests provided: [scripts/regression_small.sh](scripts/regression_small.sh), that executes a small test-case, and [scripts/regression_large.sh](scripts/regression_large.sh), that executes a large test-case. Each test executes the corresponding test-case using the GCC and FCC versions of ABEA. The tests check that the results are correct by comparing the obtained output with the result obtained on an Intel machine. The scripts also print the execution time of each benchmark. To execute the tests:

```
bash scripts/regression_small.sh
bash scripts/regression_large.sh
```