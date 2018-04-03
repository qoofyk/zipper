#!/usr/bin/env bash

# First created: 2018 Mar 29
# Last modified: 2018 Apr 02

# Author: Feng Li
# email: fengggli@yahoo.com

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/Stampede2.toolchain.cmake  \
    -Dall_transports=on \
    -Duse_itac=off \
    -Dbuild_zipper=off \
    -DUSE_SAME_LOCK=on 
