#!/usr/bin/env python

from pydmd import FbDMD
import numpy as np
import sys
import logging
import sys

def debug_print(data):
    chars = len(data)
    words = len(data.split())
    lines = len(data.split('\n'))

    print ("\n=========================================")
    print ("{0}   {1}   {2}".format(lines, words, chars))
    print(data)

if __name__ == '__main__':

    #logging.basicConfig(level=logging.DEBUG)

    data = sys.stdin.read()

    data = data.replace('[','').replace(']','')

    if(False):
        debug_print(data)
        sys.exit(0)

    input_lines =  data.split('\n')[:-1]
    if(len(input_lines) < 1):
        print("No input this time!")
        sys.exit(0)
    print("----- nline:", len(input_lines))

    field_len = len(input_lines[0].split(',')) - 2

    # ignore the stepid
    snapshots=np.loadtxt(input_lines, delimiter=',', usecols=range(1, field_len + 1)).T
    logging.info('input shape:' + str(snapshots.shape) + 'field_len=' + str(field_len))
    logging.info(snapshots)

    fbdmd = FbDMD(exact=True, svd_rank = 4)
    try:
        fbdmd.fit(snapshots)
    except ValueError:
        logging.warning("fit error:")
        logging.warning('input shape:' + str(snapshots.shape) + 'field_len=' + str(field_len))
        logging.warning("snapshots:" + str(snapshots))
        pass

    logging.info("reconstructed shape:" + str(fbdmd.reconstructed_data.shape) +"eigen_dist value: ")

    eigen_dist = np.sum(np.abs(fbdmd.eigs.real**2 + fbdmd.eigs.imag**2 - 1))
    print("------ ", eigen_dist)
