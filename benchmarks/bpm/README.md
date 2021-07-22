# Bit-Parallel Myers (BPM)

## Compile in AFX64:

To compile BPM you can use [scripts/compile.sh](scripts/compile.sh). By default the script compiles BPM with GCC and FCC. To execute the script:

```
pjsub --interact
bash scripts/compile.sh
```
OR
```
pjsub scripts/compile.sh
```

Alternatively, you can load the required modules and use (Makefile.gcc)[Makefile.gcc] to compile with GCC and (Makefile.fcc)[Makefile.fcc] to compile with FCC.

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

There are two regression tests provided: [scripts/regression_small.sh](scripts/regression_small.sh), that executes a small test-case, and [scripts/regression_large.sh](scripts/regression_large.sh), that executes a large test-case. Each test executes the corresponding test-case using the GCC and FCC versions of BPM. The tests check that the results are correct by comparing the obtained checksums with the results obtained on an Intel machine. The scripts also print the execution time of each benchmark. To execute the tests:

```
bash scripts/regression_small.sh
bash scripts/regression_large.sh
```
