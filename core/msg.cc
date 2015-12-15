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
#include <vector>
#include <mutex>

//Need to maintain some buffer of clauses rcvd but not yet pushed here.. ? 

using namespace Minisat;

std::vector<Lit> remote_units ;
std::vector<std::vector<Lit> > remote_clauses ;
std::mutex pull_mutex ;

//myaddr
//broadcast-list 


//Should just return the literal 
int msg_recv()
{
    pull_mutex.lock() ;
    //size,clause
    //Add to separate unit and clause vectors
    //When pull from remote is called, then
    pull_mutex.unlock() ;
    return 0; 
}


int pull_from_remote(int tid)
{
    //Separate handling of Unit and non-unit clauses. Icky....
    pull_mutex.lock() ;
    while (remote_units.size() > 1) {
    //slurp from this by popping
	Lit unit = remote_units.back();
	remote_units.pop_back() ;
    //do something with it!
    }
    //TODO: convert from std::vector to vec below. 
    std::vector<Lit> clause = remote_clauses.back() ;
    remote_clauses.pop_back();
    
    pull_mutex.unlock() ;
    
}

int broadcast_msg(void* buf)
{
    //send buffer to all remotes 
}


int push_unit_remote(Lit unit)
{
    //just a msg send after making into byte array and sending?
    vec<Lit> uc ;
    uc.push(unit) ;
    push_clause_remote(uc) ;
}


int push_clause_remote(vec<Lit>& learnt_clause)
{
    //transform the vector into a suitable byte array and then send
    int sz = learnt_clause.size() ;
    int* tosend = (int*) malloc(sizeof(int)*(sz+1));
    tosend[0] = sz*sizeof(int) ; //this has to be in bytes!
    tosend = tosend + 1 ;
    Lit lit ;
    int x ;
    for(int i = 0 ; i < sz ; i++) {
	lit = learnt_clause[i] ;
	*(tosend++) = lit.x ;
    }
    
    broadcast_msg(tosend) ;
    //once sent, free this up?
    free(tosend) ;
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

