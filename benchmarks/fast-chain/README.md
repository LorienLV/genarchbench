# Chain

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

## Compile in AFX64:

WARNING: This version of Chain uses uint_32 elements instead of uint_64 as the original CHAIN.

To compile CHAIN you can use [scripts/compile.sh](scripts/compile.sh). By default the script compiles CHAIN with GCC and FCC. To execute the script:

```
pjsub --interact
bash scripts/compile.sh
```
OR
```
pjsub scripts/compile.sh
```

Alternatively, you can load the required modules and use (Makefile.gcc)[Makefile.gcc] to compile with GCC and (Makefile.fcc)[Makefile.fcc] to compile with FCC.

## Run regression tests

There are two regression tests provided: [scripts/regression_small.sh](scripts/regression_small.sh), that executes a small test-case, and [scripts/regression_large.sh](scripts/regression_large.sh), that executes a large test-case. Each test executes the corresponding test-case using the GCC and FCC versions of CHAIN with different combinations of OMP threads. The tests check that the results are correct by comparing the obtained output with the results obtained on an Intel machine. The scripts also print the execution time of each benchmark. To execute the tests:

```
bash scripts/regression_small.sh
bash scripts/regression_large.sh
```
