#!/bin/bash

for j in 10000 100000 500000 750000 1000000
do
        sed -i "s/#define LINES.*/#define LINES $j/" mainpthread.c
        gcc -pthread -o pthread$j mainpthread.c

        for i in 1 2 4 8 16 
        do
                sbatch --constraint=elves --time=00:01:00 --cpus-per-task=$i --nodes=1 --mem-per-cpu=4G run.sh $j
        done
done

