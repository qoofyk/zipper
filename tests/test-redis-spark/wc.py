#!/usr/bin/env python

import sys

if __name__ == '__main__':

    data = sys.stdin.read()
    chars = len(data)
    words = len(data.split())
    lines = len(data.split('\n'))

    print ("{0}   {1}   {2}".format(lines, words, chars))
    print(data)
