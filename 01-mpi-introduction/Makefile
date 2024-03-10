CC=mpicc
all: build
build:
	mkdir -p build
	$(CC) hello.c -o build/hello
	$(CC) reduce.c -o build/reduce
run-hello:
	mpirun build/hello
run-reduce:
	mpirun -np 4 build/reduce -n 10000
