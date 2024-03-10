#include <stdio.h>
#include <mpi.h>

int main (int Argc, char* Argv[])
{
  int ErrCode = MPI_Init(&Argc, &Argv);
  if (ErrCode != 0) {
		fprintf(stderr, "Error: Failed to initialize MPI!");
    return ErrCode;
	}
  int Rank, Size;
	MPI_Comm_size(MPI_COMM_WORLD, &Size);
  MPI_Comm_rank(MPI_COMM_WORLD, &Rank);  
	printf("Hello, World! Size: %d, Rank: %d\n", Rank, Size);
  MPI_Finalize();
  return 0;
}
