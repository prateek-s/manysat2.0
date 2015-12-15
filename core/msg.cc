#include "core/msg.h"
#include <stdio.h>
//MPI STUFF? 

//Need to maintain some buffer of clauses rcvd but not yet pushed here.. ? 
//using namespace Minisat;

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


int msg_send(int from, int lit)
{
    //MPI_Init(NULL, NULL) ;
    //Should be a non-blocking send since its called from coop:exportunit
    //MPI broadcast or send? can be MPI_Int maybe
    //printf("msg_send[%d]: %d\n", from, lit);
    return 0;
}

//Should just return the literal 
int msg_recv()
{
    return 0; 
}

//vec<Lit> get_remote_units()
//{
    //Just return the vector
    //Garbage collection etc later??
    //Keep a counter or something?
    //
//}


int msg_main_loop()
{
    printf("----------------- MAIN LOOP CALLED ! \n") ;
    fflush(NULL);
    
    while(1) {
	 msg_recv() ; //can/should be blocking? 
	//save r in some local vector?
	//push to local threads later? 
    }

}

