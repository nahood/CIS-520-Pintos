#!/bin/bash

linecount=1000000
lc=$(($linecount - 1))

echo "" > "traversed"

echo "" > "finaldiff.txt"

for i in slurm*
do
	hd=`head -qn$lc $i`
	echo "$hd" > "head$i"
	
done

for i in head*
do
	for j in head*
	do
		if [ $i != $j ] && [[ `cat traversed | grep "$i"` != "$i" ]]
		then
			diff -qs $i $j 
			`diff -u $i $j` >> "finaldiff.txt"
		fi
	done
	echo "$i" >> "traversed"
done

rm "traversed"

for i in slurm*
do
	rm "head$i"
done

echo "Check finaldiff.txt for diffs if there are any outputs that do not match."
