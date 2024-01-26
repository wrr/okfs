#!/bin/sh

#if [ $# != 2 ];  then
#    echo "usage: $0";
#    exit;
#fi

#sto milionow
#for((i=0; i <= 1000000; i+=100000))

res=read_okfs_localfs_short;
> $res;

for((k = 10000; k <= 100000; k+=10000))
  do
  > tmp;
  for((i=0; i <= 9; i++))
    do
    ./a.out $k >> tmp;
  done
  echo -n "$k " >> $res;
  ./mediana.py <tmp >>$res;
done
  
