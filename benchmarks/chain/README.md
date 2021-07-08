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

```
pjsub --interact
bash scripts/compile.sh -c "compiler (gcc or fcc)"
```
OR
```
pjsub scripts/compile.sh -c "compiler (gcc or fcc)"
```

## Execute

Download the inputs [here](https://genomicsbench.eecs.umich.edu/input-datasets.tar.gz).

```
pjsub --interact
bash scripts/run.sh -i "inputs folder path" -s "size of the input (small or large)" -c "compiler (gcc or fcc)"
```
OR
```
pjsub scripts/run.sh -i "inputs folder path" -s "size of the input (small or large)" -c "compiler (gcc or fcc)"
```

You can set the number of threads of the execution modifying the run.sh script.
