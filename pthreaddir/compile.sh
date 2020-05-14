#!/bin/bash

name="results:`date "+%F-%T"`"
cores=1
nextcores=2
lines=10000
nextlines=100000
touch $name

for i in slurm*
do
		
		if [[ $cores -eq 1 ]]
		then
			echo "" >> $name
			echo "" >> $name
			echo "------------------------" >> $name
			echo "Number of Lines: $lines" >> $name
			echo "------------------------" >> $name
			
			if [[ $lines -eq 10000 ]]
			then
				nextlines=100000
			fi
			if [[ $lines -eq 100000 ]]
			then
				nextlines=500000
			fi
			if [[ $lines -eq 500000 ]]
			then
				nextlines=750000
			fi
			if [[ $lines -eq 750000 ]]
			then
				nextlines=1000000
			fi
			if [[ $lines -eq 1000000 ]]
			then
				nextlines=10000
			fi
			
			echo "" >> $name
			echo "Number of Cores: $cores" >> $name
			nextcores=2
		fi
		if [[ $cores -eq 2 ]]
		then
			echo "" >> $name
			echo "Number of Cores: $cores" >> $name
			nextcores=4
		fi
		if [[ $cores -eq 4 ]]
		then
			echo "" >> $name
			echo "Number of Cores: $cores" >> $name
			nextcores=8
		fi
		if [[ $cores -eq 8 ]]
		then
			echo "" >> $name
			echo "Number of Cores: $cores" >> $name
			nextcores=16
		fi
		if [[ $cores -eq 16 ]]
		then
			echo "" >> $name
			echo "Number of Cores: $cores" >> $name
			nextcores=1
			lines=$nextlines
		fi

		tl=`tail -qn5 $i`
		echo "$tl" >> $name
		cores=$nextcores
done
echo "Results compiled in $name"
cat $name

echo "`cat $name | grep "Total running time: "`" >> "$name".out
sed -i "s/Total running time: //" "$name".out
