// ----------------------------------------------------------------------------
//   "THE BEER-WARE LICENSE" (Revision 42):
//   <alex.rom23@mail.ru> wrote this file. As long as you retain this
//   notice you can do whatever you want with this stuff. If we meet some day,
//   and you think this stuff is worth it, you can buy me a beer in return.
// ----------------------------------------------------------------------------

#include <mpi.h>
#include <stdio.h>

int main(int Argc, char *Argv[]) {
  int ErrCode = MPI_Init(&Argc, &Argv);
  if (ErrCode != 0) {
    fprintf(stderr, "Error: Failed to initialize MPI!");
    return ErrCode;
  }
  int Rank, Size;
  MPI_Comm_size(MPI_COMM_WORLD, &Size);
  MPI_Comm_rank(MPI_COMM_WORLD, &Rank);
  printf("Hello, World! Size: %d, Rank: %d\n", Size, Rank);
  MPI_Finalize();
  return 0;
}
