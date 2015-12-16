/* Header file for the message passing functions. MPI? */

//#include <mpi.h>
#include "mtl/Vec.h"
#include "mtl/Heap.h"
#include "mtl/Alg.h"
#include "utils/Options.h"

 #include "core/SolverTypes.h" 
 #include "core/Solver.h" 
 #include "core/Cooperation.h" 

using namespace Minisat;

int mpi_init(int argc, char* argv[]) ;

int msg_send(int from, int lit) ;

int msg_recv() ;

int msg_main_loop();

//vec<vec<Lit> > pull_clauses_from_remote(int tid) ;
vec<Lit> pull_units_from_remote(int tid);
void pull_clauses_from_remote(Solver* s, Cooperation* coop, int tid) ;


int push_unit_remote(Lit unit) ;

int push_clause_remote(vec<Lit>& learnt_clause);
