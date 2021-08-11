#!/bin/bash

DEPDIR=$(pwd)
LOCAL=$DEPDIR/usr/local

mkdir -p $LOCAL

# sundials
SUNDIALS=$LOCAL/sundials
mkdir -p sundials

cd sundials

git clone https://github.com/LLNL/sundials.git .
cmake -B build -DCMAKE_INSTALL_PREFIX=$SUNDIALS  -DENABLE_OPENMP=OFF .
cd build
make
make install
make test

cd ..

