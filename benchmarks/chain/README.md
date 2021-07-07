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

### GCC

```
module load gcc/10.2.0
make -f Makefile.gcc
```

### FCC

```
module load fuji
make -f Makefile.fcc
```

## Execute

Download the inputs [here](https://genomicsbench.eecs.umich.edu/input-datasets.tar.gz).

### Execute small input

`./chain -i inputs/small/in-1k.txt -o out-small.txt`

### Execute large input

`./chain -i inputs/large/c_elegans_40x.10k.txt -o out-large.txt`
