#!/usr/bin/env bash

type=$1
if [ ${type} -z ];then
    type=1
fi

#if [ ${type} -eq 0 ];then
    rm -rf vps
#else
    rm -rf libVPSNativelib.dylib
#fi

mkdir build
cd build

cmake -DSHARED_LIBRARY=${type} ../ 

make

if [ ${type} -eq 0 ];then
    cp vps ../
else
    cp libVPSNativelib.dylib ../
fi

cd ..

rm -rf build