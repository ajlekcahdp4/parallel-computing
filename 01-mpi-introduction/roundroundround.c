// ----------------------------------------------------------------------------
//   "THE BEER-WARE LICENSE" (Revision 42):
//   <alex.rom23@mail.ru> wrote this file. As long as you retain this
//   notice you can do whatever you want with this stuff. If we meet some day,
//   and you think this stuff is worth it, you can buy me a beer in return.
// ----------------------------------------------------------------------------

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <mpi.h>

int changeValue(int Val, int Rank) {
  printf("Rank: %d, CurrVal: %d\n", Rank, Val);
  return ++Val;
}

#define ROOT_RANK 0

int main(int Argc, char **Argv) {
  int ErrCode = MPI_Init(&Argc, &Argv);
  if (ErrCode != 0) {
    fprintf(stderr, "Error: Failed to initialize MPI!");
    return ErrCode;
  }
  int Rank = 0, PNum = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &PNum);
  MPI_Comm_rank(MPI_COMM_WORLD, &Rank);
  int N = 0;
  if (Rank == ROOT_RANK) {
    N = changeValue(N, Rank);
    MPI_Send(&N, 1, MPI_INT, Rank + 1, 0, MPI_COMM_WORLD);
  } else {
    MPI_Recv(&N, 1, MPI_INT, Rank - 1, 0, MPI_COMM_WORLD, NULL);
    N = changeValue(N, Rank);
    MPI_Send(&N, 1, MPI_INT, (Rank + 1) % PNum, 0, MPI_COMM_WORLD);
  }
  if (Rank == ROOT_RANK) {
    MPI_Recv(&N, 1, MPI_INT, PNum - 1, 0, MPI_COMM_WORLD, NULL);
    N = changeValue(N, Rank);
  }
  MPI_Finalize();
  return 0;
}
