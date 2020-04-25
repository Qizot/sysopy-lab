#!/bin/bash
for line in $(ipcs | grep 42050002 | awk '{print $2}'); do
	num=$(($line + 0))
	echo $num;
	if [ $num -gt 1000 ]; then
		ipcrm -m $num;
	fi
	if [ $num -lt 1000 ]; then
		ipcrm -s $num;
	fi
done
