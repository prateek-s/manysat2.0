/* Header file for the message passing functions. MPI? */

//#include <mpi.h>
#include "mtl/Vec.h"
#include "mtl/Heap.h"
#include "mtl/Alg.h"
#include "utils/Options.h"
#include "core/SolverTypes.h"

using namespace Minisat;

int mpi_init(int argc, char* argv[]) ;

int msg_send(int from, int lit) ;

int msg_recv() ;

int msg_main_loop();


int pull_from_remote(int tid);

int push_unit_remote(Lit unit) ;

int push_clause_remote(vec<Lit>& learnt_clause);
