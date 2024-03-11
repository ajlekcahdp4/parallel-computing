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

unsigned long long getNumber(int Argc, char **Argv) {
  int Res = 0, Opt = 0;
  while ((Opt = getopt(Argc, Argv, "n:")) != -1) {
    switch (Opt) {
    case 'n':
      Res = atol(optarg);
      break;
    default:
      fprintf(stderr, "Error: Invalid argument!");
      exit(EXIT_FAILURE);
    }
  }
  return Res;
}

#define ROOT_RANK 0

int main(int Argc, char **Argv) {
  unsigned long long N = getNumber(Argc, Argv);

  int ErrCode = MPI_Init(&Argc, &Argv);
  if (ErrCode != 0) {
    fprintf(stderr, "Error: Failed to initialize MPI!");
    return ErrCode;
  }
  int Rank = 0, PNum = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &PNum);
  MPI_Comm_rank(MPI_COMM_WORLD, &Rank);
  unsigned long long PerProc = N / PNum;
  double LocalRes = 0.0;
  if (Rank != PNum - 1)
    for (unsigned I = 1; I <= PerProc; ++I) {
      LocalRes += 1.0 / (I * Rank + 1);
    }
  else
    for (unsigned I = PerProc * PNum; I <= N; ++I) {
      LocalRes += 1.0 / I;
    }
  double Reduced = 0.0;
  MPI_Reduce(&LocalRes, &Reduced, 1 /* Num Sent*/, MPI_DOUBLE, MPI_SUM,
             ROOT_RANK, MPI_COMM_WORLD);
  double Result = Reduced * N;
  if (Rank == ROOT_RANK)
    printf("Rank: %d, Sum over N: %lld, Result: %lf\n", Rank, N, Result);
  MPI_Finalize();
  return EXIT_SUCCESS;
}
