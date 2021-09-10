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

## Compile in AFX64:

To compile KMER-CNT you can use [scripts/compile.sh](scripts/compile.sh). By default the script compiles KMER-CNT with GCC and FCC. To execute the script:

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

There are two regression tests provided: [scripts/regression_small.sh](scripts/regression_small.sh), that executes a small test-case, and [scripts/regression_large.sh](scripts/regression_large.sh), that executes a large test-case. Each test executes the corresponding test-case using the GCC and FCC versions of KMER-CNT with different combinations of OMP threads. The tests check that the results are correct by comparing the number of k-mers counted with the reference value obtained on an Intel machine. The scripts also print the execution time of each benchmark. To execute the tests:

```
bash scripts/regression_small.sh
bash scripts/regression_large.sh
```

