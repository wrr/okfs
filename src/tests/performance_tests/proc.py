#!/usr/bin/python -tt
import sys

linux = float(sys.argv[1])
okfs = float(sys.argv[2])

print "Linux " + str(linux) + " OKFS " +str(okfs)
print str(100.0 * (okfs - linux) / linux)
