#!/usr/bin/env bash

echo "Measuring 3E"
echo "Sequential:"
build/parloop-omp --parallel off --isize $1 --jsize $1
echo "OMP:"
for n in {1..8}; do
  echo "For $n threads:"
  build/parloop-omp  --parallel on -n $n --isize $1 --jsize $1
done

echo -e "\n\n"

echo "Measuring 2Д"
echo "Sequential:"
build/parloop-mpi --parallel off --isize $1 --jsize $1
echo "OMP:"
for n in 1 2 4; do
  echo "For $n threads:"
  mpirun -n $n build/parloop-mpi  --parallel on --isize $1 --jsize $1
done
