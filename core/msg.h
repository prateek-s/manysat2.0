/* Header file for the message passing functions. MPI? */

//#include <mpi.h>

int mpi_init(int argc, char* argv[]) ;

int msg_send(int from, int lit) ;

int msg_recv() ;

int msg_main_loop();

