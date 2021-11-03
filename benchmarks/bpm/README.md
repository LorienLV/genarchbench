# Bit-Parallel Myers (BPM)

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
