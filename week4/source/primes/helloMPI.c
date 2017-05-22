/* C Example */
#include <mpi.h>
#include <stdio.h>

#define MASTER 0
#define SINGLE 1


int main (int argc, char* argv[])
{
  int rank, size;
 
  MPI_Init (&argc, &argv);      /* starts MPI */
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);        /* get current process id */
  MPI_Comm_size (MPI_COMM_WORLD, &size);        /* get number of processes */
  if (rank == MASTER) {
    for (int i = 0; i < argc; ++i) {
      printf("argv[%d] = %s\n", i, argv[i]);
    }
  }
  printf( "Hello world from process %d of %d, lets get shit done yo!\n", rank, size );



  MPI_Finalize();
  return 0;
}
