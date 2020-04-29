#!/bin/bash
LOG=Times.txt
echo "Testing $2 image" >> $LOG 
printf "\n\n" >> $LOG

threads=(1 2 4 8)
modes=(sign block interleaved)

for t in "${threads[@]}"
do
	for mode in "${modes[@]}"
	do
		echo "Mode: $mode Threads: $t" >> $LOG
		./$1 $t $mode $2 tmp.txt >> $LOG
		rm tmp.txt
		printf "\n\n" >> $LOG
	done
done

