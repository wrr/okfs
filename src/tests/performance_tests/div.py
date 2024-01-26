#!/usr/bin/python -tt
import sys

l1 = open("write_okfs_localfs_short").read().split()
l2 = open("write_okfs_shfs").read().split()

for i in range(0,20,2):
#    print l1[i] + " " + str(100.0 * (float(l2[i + 1]) - float(l1[i + 1])) / float(l1[i + 1]))
    print l1[i] + " " + str( (float(l2[i + 1]) / float(l1[i + 1])));
#     print l1[i] + " " + str(float(l2[i + 1]) - float(l1[i + 1]));

