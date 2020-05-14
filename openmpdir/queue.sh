#!/bin/bash

#sed -i "s/#define LINES.*/#define LINES 10000/" mainopenmp.c
#sed -i "s/#define LINES.*/#define LINES 100000/" mainopenmp.c
#sed -i "s/#define LINES.*/#define LINES 500000/" mainopenmp.c
#sed -i "s/#define LINES.*/#define LINES 750000/" mainopenmp.c
#sed -i "s/#define LINES.*/#define LINES 1000000/" mainopenmp.c

for j in 10000 100000 500000 750000 1000000
do
	sed -i "s/#define LINES.*/#define LINES $j/" mainopenmp.c
	gcc -fopenmp -o openmp$j mainopenmp.c

	for i in 1 2 4 8 16
	do
		sbatch --constraint=elves --time=00:01:00 --cpus-per-task=$i --nodes=1 --mem-per-cpu=4G run.sh $j
	done
done
