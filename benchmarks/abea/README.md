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

### GCC

```
module load gcc/10.2.0
make -f Makefile.gcc
```

### FCC

```
module load fuji
# To prevent this error: Catastrophic error: could not set locale "" to allow processing of multibyte characters
export LANG=en_US.utf8
export LC_ALL=en_US.utf8
make -f Makefile.fcc
```
