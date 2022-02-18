#!/bin/bash

#PJM -N CHAIN.COMPILE
#PJM -L elapse=00:30:00
#PJM -L node=1
#PJM --mpi "proc=1,max-proc-per-node=1"

export OMP_NUM_THREADS=1

# The compiler to use, "gcc" and/or "fcc".
compilers=(
    'gcc'
    'fcc'
)

for compiler in "${compilers[@]}"; do
    echo "----- Compiling with $compiler -----"

    build_dir="2.0/build_dynamic_$compiler"
    mkdir "$build_dir"
    cp -r "2.0/build_dynamic/." "$build_dir"

    pushd "$build_dir"

    case "$compiler" in
        gcc)
            module load gcc/10.2.0

            # make CC=gcc CXX=g++ arch='-march=armv8-a+sve -I/opt/FJSVxtclanga/tcsds-1.2.26b/clang-comp/include/external/' \
            # LAPACK_FLAG='-L/apps/ARM/20.3/armpl-20.3.0_ThunderX2CN99_RHEL-8_arm-linux-compiler_aarch64-linux/lib/ -larmpl -L/apps/ARM/20.3/arm-linux-compiler-20.3_Generic-AArch64_RHEL-8_aarch64-linux/lib/ -lflangrti-omp -L/apps/ARM/20.3/arm-linux-compiler-20.3_Generic-AArch64_RHEL-8_aarch64-linux/lib/ -lamath_a64fx -Wl,-rpath-link=/apps/ARM/20.3/arm-linux-compiler-20.3_Generic-AArch64_RHEL-8_aarch64-linux/lib/ -Wl,-rpath-link=/apps/ARM/20.3/arm-linux-compiler-20.3_Generic-AArch64_RHEL-8_aarch64-linux/lib/' \
            # BLAS_FLAG='' ATLAS_FLAG=''

            # make CC=gcc CXX=g++ \
            # arch='-march=armv8-a+sve -DLAPACK_ILP64 -I/opt/FJSVxtclanga/tcsds-1.1.18/include/lapack_ilp64 -fopenmp' \
            # BLAS_FLAG='/opt/FJSVxtclanga/tcsds-1.1.18/lib64/libfjlapacksve_ilp64.so' LAPACK_FLAG='' ATLAS_FLAG=''
            # # LAPACK_FLAG='-SSL2'

            make CC=gcc CXX=g++ \
            arch="-march=armv8-a+sve \
                  -I/fefs/scratch/bsc18/bsc18248/lapack-3.10.0/CBLAS/include" \
            BLAS_FLAG="/fefs/scratch/bsc18/bsc18248/lapack-3.10.0/libcblas.a \
                       /fefs/scratch/bsc18/bsc18248/lapack-3.10.0/libblas.a" \
            LAPACK_FLAG="/fefs/scratch/bsc18/bsc18248/lapack-3.10.0/liblapack.a -lgfortran" \
            ATLAS_FLAG='' \
            DYNAMORIO_ANALYSIS=1
            ;;
        fcc)
            module load fuji

            make CC='fcc -Nclang' CXX='FCC -Nclang' \
            arch="-march=armv8-a+sve \
                  -I/fefs/scratch/bsc18/bsc18248/lapack-3.10.0/CBLAS/include" \
            BLAS_FLAG="/fefs/scratch/bsc18/bsc18248/lapack-3.10.0/libcblas.a \
                       /fefs/scratch/bsc18/bsc18248/lapack-3.10.0/libblas.a" \
            LAPACK_FLAG="/fefs/scratch/bsc18/bsc18248/lapack-3.10.0/liblapack.a -lgfortran" \
            ATLAS_FLAG='' \
            FAPP_ANALYSIS=1 DYNAMORIO_ANALYSIS=1
            ;;
        *)
            echo "ERROR: Compiler '$compiler' not supported."
            exit 1
    esac

    popd
done
