CC=mpicc
all: build
build:
	mkdir -p build
	$(CC) hello.c -o build/hello

run-hello:
	mpirun build/hello
