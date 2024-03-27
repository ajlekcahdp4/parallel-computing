echo "sequential:"
time build/sort --number $2 > /dev/null
echo "parallel"
time mpirun -n $1 build/sort --number $2 --parallel=true > /dev/null
