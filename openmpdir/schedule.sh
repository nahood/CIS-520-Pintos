#!/bin/bash

make

for i in 1 2 4 8 16 
do
	sbatch --constraint=elves --time=00:01:00 --cpus-per-task=$i --nodes=1 --mem-per-cpu=4G run.sh
done
