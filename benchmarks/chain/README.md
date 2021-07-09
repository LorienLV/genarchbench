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

1. Select the compiler to use by modifying the `compiler` variable in `scripts/compile.sh`. 
2. Execute the script:
    ```
    pjsub --interact
    bash scripts/compile.sh
    ```
    OR
    ```
    pjsub scripts/compile.sh
    ```

## Execute

1. Download the inputs [here](https://genomicsbench.eecs.umich.edu/input-datasets.tar.gz).

2. Select the number of threads, inputs folder, compiler to use, and input to use by modifying the required variables in `scripts/run.sh`. 

3. Execute the script:
    ```
    pjsub --interact
    bash scripts/run.sh
    ```
    OR
    ```
    pjsub scripts/run.sh
    ```

