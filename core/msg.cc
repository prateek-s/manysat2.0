#include <mpi.h>
#include "core/msg.h"
#include <stdio.h>
//MPI STUFF? 

int mpi_init(int argc, char* argv[])
{
  /* int rank, size; */
 
  /* MPI_Init (&argc, &argv);      /\* starts MPI *\/ */
  /* MPI_Comm_rank (MPI_COMM_WORLD, &rank);        /\* get current process id *\/ */
  /* MPI_Comm_size (MPI_COMM_WORLD, &size);        /\* get number of processes *\/ */
  /* printf( "Hello world from process %d of %d\n", rank, size ); */
  /* MPI_Finalize(); */
  return 0;
}


int msg_send(int lit)
{
    MPI_Init(NULL, NULL);
    printf("msg send called\n");
    return 0;
}

int msg_recv(int *lit)
{
    return 0; 
}
