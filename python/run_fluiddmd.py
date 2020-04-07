#!/usr/bin/env python

from pydmd import FbDMD
import numpy as np
import sys
import logging
import sys

if __name__ == '__main__':

    logging.basicConfig(level=logging.DEBUG)

    data = sys.stdin.read()
    input_lines =  data.split('\n')[:-1]
    if(len(input_lines) < 1):
        print("No input this time!")
        sys.exit(0)
    else:
        print(input_lines)

    # ignore the stepid
    snapshots=np.loadtxt((line.split(' ')[1] for line in input_lines), delimiter=',').T
    logging.info('input shape:' + str(snapshots.shape))
    #logging.info(snapshots)

    fbdmd = FbDMD(exact=True)
    fbdmd.fit(snapshots)
    logging.info("reconstructed shape:" + str(fbdmd.reconstructed_data.shape) +"eigen_dist value: ")

    eigen_dist = np.sum(np.abs(fbdmd.eigs.real**2 + fbdmd.eigs.imag**2 - 1))
    print(eigen_dist)
