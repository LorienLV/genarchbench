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

1. Execute `compile.sh` to compiler with both GCC and FCC:
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

2. Before executing you have to generate the index files for the input .fastq files. Set the inputs folder, and the binary to use modifying the `inputs_path` and `compiler` variable in `scripts/config.sh`. Then execute the script:
    ```
    pjsub --interact
    bash scripts/config.sh
    ```
    OR
    ```
    pjsub scripts/config.sh
    ```

2. Set the inputs folder in `scripts/regression_small.sh` and `scripts/regression_large.sh`. 

3. Execute the scripts:
    ```
    pjsub --interact
    bash scripts/run.sh
    bash scripts/run.sh
    ```
    OR
    ```
    pjsub scripts/run.sh
    ```
