#!/bin/sh
#$Id: testall.sh,v 1.4 2006/05/06 22:14:46 jan_wrobel Exp $

#run all tests in current directory

echo "TESTING GENERAL CALLS:"
echo " ";
succ=0;
fail=0;

../okfs -h 2>/dev/null;

echo -n "executing okfs interruption test";
error=`exec ../okfs -r ./okfs_test_int 2>&1`;
ret_val=$?;
if [ ! -z "$error" ]; then
    echo " --->failed: $error;";
    ((fail++));
elif [ $ret_val -ne "1" ]; then
    ((fail++));
    echo " --->failed: okfs returned" $ret_val " instead of 1";
else
    ((succ++));
    echo " --->passed";
fi

for test in *_test; do
    echo -n "executing" $test; 
    error=`exec ../okfs -r ./$test 2>&1`;
    ret_val=$?;
    if [ ! -z "$error" ]; then
	echo " --->failed: $error;";
	((fail++));
    elif [ $ret_val -ne "0" ]; then
       	((fail++));
	echo " --->failed: okfs returned" $ret_val;
    else
	((succ++));
	echo " --->passed";
    fi
done

echo "$PWD/localfs_test_dir	/etc	localfs" >test_fstab;

echo "END OF GENERAL CALLS TESTS"
echo " "
echo "TESTING FILE SYSTEM SPECIFIED CALLS:"
echo " "

for test in *_test_fs; do
    echo -n "executing" $test; 
    error=`exec ../okfs -r -t /tmp -f ./test_fstab ./$test 2>&1`;
    ret_val=$?;
    if [ ! -z "$error" ]; then
	echo " --->failed: $error";
	((fail++));
    elif [ $ret_val -ne "0" ]; then
        echo " --->failed: okfs returned" $ret_val;
	((fail++));
    else
	echo " --->passed";
	((succ++));
    fi;
done
echo "END OF FILE SYSTEM SPECIFIED CALLS TESTS"
echo " "
echo "TESTS REPORT"
echo "$fail tests failed";
echo "$succ tests passed";
if (($fail)) ; then
    echo "TESTING FAILED";
    exit 1;
fi;
echo "TESTING PASSED";