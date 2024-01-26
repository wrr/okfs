#!/usr/bin/python -tt

import sys

a = sys.stdin.readline()
val = []
i = 0
s=0.0;

while a:
    val.append(float(a))

    a = sys.stdin.readline()


val.sort()
print >>sys.stderr, val
val = val[2:-2]
print >>sys.stderr, val

for a in val:
    i += 1
    s += a

#print val[len(val)/2 - 1]
print s/i
