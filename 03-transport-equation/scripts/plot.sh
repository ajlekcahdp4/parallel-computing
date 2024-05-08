#!/usr/bin/env bash
mpirun -n 4 build/solution --grid-size=$1 --parallel=true --print=true > tmp.txt
gnuplot -e "filename='tmp.txt'" scripts/gnuplot.sh
rm -f tmp.txt
