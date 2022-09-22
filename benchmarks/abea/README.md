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

## Running

Command line: `./f5c eventalign -b <bam> -g <ref genome fasta> -r <fastq reads>`

IMPORTANT: If you have moved the input files to a new location, you must regenerate the index files:
```
inputs_path="$GENARCH_BENCH_INPUTS_ROOT/abea"
f5c index -d "$inputs_path"/fast5_files "$inputs_path"/1000reads.fastq
f5c index -d "$inputs_path"/fast5_files "$inputs_path"/10000reads.fastq
```
