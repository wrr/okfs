#!/bin/sh

if [ $# != 1 ];  then
    echo "usage: $0";
    exit;
fi

#rm -rf /etc/tfoo /etc/tbar;
dd if=/dev/zero of=/etc/tfoo ibs=$1 obs=$1 count=1 2>/dev/null;
sync;
cp /etc/tfoo /etc/tbar;
#sync;
#diff /etc/tfoo /etc/tbar;

