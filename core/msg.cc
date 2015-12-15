#include "core/msg.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <endian.h>
#include <iostream>
//MPI STUFF? 

//Need to maintain some buffer of clauses rcvd but not yet pushed here.. ? 

using namespace Minisat;

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
    //for loop which sends via all sockets, yes!
    return 0;
}

//Should just return the literal 
int msg_recv()
{
    return 0; 
}


int pull_from_remote(int tid)
{
    //Separate handling of Unit and non-unit clauses. Icky.... 
}

int push_unit_remote(Lit unit)
{

}


int push_clause_remote(vec<Lit>& learnt_clause)
{

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
    //Open sockets via the others and me file
    //After that, start the sock_recv loop 
    while(1) {
	 msg_recv() ; //can/should be blocking?
	 //socket recv and 
	//save r in some local vector?
	//push to local threads later? 
    }

}

