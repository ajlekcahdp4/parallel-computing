#!/usr/bin/env bash
echo "Sequential time:"
mpirun -n 1 build/solution --grid-size=$1 --parallel=true --measure=true
echo "Parallel run:"
mpirun -n $2 build/solution --grid-size=$1 --parallel=true --measure=true
