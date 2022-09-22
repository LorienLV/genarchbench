# Neural Network-based Base Calling (NN-BASE)

`NN-BASE` uses the same license as [Bonito](https://github.com/nanoporetech/bonito).

## Requirements

To run nn-base you must install the required python packages: `pip install -r requirements.txt`

## Execution

```
./basecall_wrapper python3 ./bonito/basecall.py <model> <reads> --chunksize <chunk-size (3000 recommended)> --fastq > out.fastq
```