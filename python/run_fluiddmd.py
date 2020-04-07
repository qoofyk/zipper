#!/usr/bin/env python

from pydmd import FbDMD
import numpy as np
import sys
import logging

if __name__ == '__main__':

    logging.basicConfig(level=logging.DEBUG)

    # ignore the stepid
    snapshots=np.loadtxt((line.split(' ')[1] for line in sys.stdin), delimiter=',').T
    logging.info('input shape:' + str(snapshots.shape))
    #logging.info(snapshots)

    fbdmd = FbDMD(exact=True)
    fbdmd.fit(snapshots)
    logging.info("reconstructed shape:" + str(fbdmd.reconstructed_data.shape) +"eigen_dist value: ")

    eigen_dist = np.sum(np.abs(fbdmd.eigs.real**2 + fbdmd.eigs.imag**2 - 1))
    print(eigen_dist)
