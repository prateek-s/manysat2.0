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
#include <string>
#include <omp.h>

//Need to maintain some buffer of clauses rcvd but not yet pushed here.. ? 

using namespace Minisat;


struct my_net_addr {
    std::string addr ;
    int port ;
};
    

std::vector<Lit> remote_units ;
std::vector<std::vector<Lit> > remote_clauses ;
std::mutex pull_mutex ;

omp_lock_t _pull_lock;
omp_lock_t _push_lock;

struct my_net_addr myaddr ;

std::vector<my_net_addr> other_addrs;

std::vector<int> broadcast_socketfds;

/* Read 'n' bytes from 'fd' into 'buf', restarting after partial
   reads or interruptions by a signal handlers */

/*******************************************************************************/

void pull_lock()
{
    omp_set_lock(&_pull_lock);
}

void pull_unlock()
{
    omp_unset_lock(&_pull_lock);
}


void push_lock()
{
    omp_set_lock(&_push_lock);
}

void push_unlock()
{
    omp_unset_lock(&_push_lock);
}

/*******************************************************************************/

ssize_t readn(int fd, void *buffer, size_t n)
{
    ssize_t numRead;                    /* # of bytes fetched by last read() */
    size_t totRead;                     /* Total # of bytes read so far */
    char *buf;

    buf = (char*) buffer;                       /* No pointer arithmetic on "void *" */
    for (totRead = 0; totRead < n; ) {
        numRead = read(fd, buf, n - totRead);

        if (numRead == 0)               /* EOF */
            return totRead;             /* May be 0 if this is first read() */
        if (numRead == -1) {
            if (errno == EINTR)
                continue;               /* Interrupted --> restart read() */
            else
                return -1;              /* Some other error */
        }
        totRead += numRead;
        buf += numRead;
    }
    return totRead;                     /* Must be 'n' bytes if we get here */
}

/* Write 'n' bytes to 'fd' from 'buf', restarting after partial
   write or interruptions by a signal handlers */

ssize_t writen(int fd, const void *buffer, size_t n)
{
    ssize_t numWritten;                 /* # of bytes written by last write() */
    size_t totWritten;                  /* Total # of bytes written so far */
    const char *buf;

    buf = (char*) buffer;                       /* No pointer arithmetic on "void *" */
    for (totWritten = 0; totWritten < n; ) {
        numWritten = write(fd, buf, n - totWritten);

        /* The "write() returns 0" case should never happen, but the
           following ensures that we don't loop forever if it does */

        if (numWritten <= 0) {
            if (numWritten == -1 && errno == EINTR)
                continue;               /* Interrupted --> restart write() */
            else
                return -1;              /* Some other error */
        }
        totWritten += numWritten;
        buf += numWritten;
    }
    return totWritten;                  /* Must be 'n' bytes if we get here */
}

/*******************************************************************************/

//or maybe OpenMP locks http://stackoverflow.com/questions/2396430/how-to-use-lock-in-openmp

//myaddr
//broadcast-list 


//Should just return the literal 
int msg_recv()
{
    return 0; 
}


/* Size of clause, and an array of integers */
int process_new_clause(int size, int* litbuf)
{
    pull_lock();
    //size,clause
    if(size == 1) {
//	remote_units.push(mklit(*litbuf));
    }
    //Add to separate unit and clause vectors
    //When pull from remote is called, then
    pull_unlock();
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
    return 0 ;
    
}

/*******************************************************************************/

int broadcast_msg(void* buf, int sz)
{
    //mutex lock here? 
    //send buffer to all remotes
    for(std::vector<int>::iterator it = broadcast_socketfds.begin(); it != broadcast_socketfds.end(); ++it) {
    /* std::cout << *it; ... */
	int socketfd = *it;
	writen(socketfd, buf, sz); 
    }
    return 0 ;
}


int push_unit_remote(Lit unit)
{
    //just a msg send after making into byte array and sending?
    vec<Lit> uc ;
    uc.push(unit) ;
    push_clause_remote(uc) ;
    return 0 ;
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
    
    broadcast_msg(tosend, sizeof(int)*(sz+1)) ;
    //once sent, free this up?
    free(tosend) ;
    return 0;
}

/*******************************************************************************/

//vec<Lit> get_remote_units()
//{
    //Just return the vector
    //Garbage collection etc later??
    //Keep a counter or something?
    //
//}

//read the files and fill in the corresponding structures
void read_network_info()
{
    
    //Also keep the sockets open! No need to close them!!
}

/*******************************************************************************/



int msg_main_loop()
{
    printf("----------------- MAIN LOOP CALLED ! \n") ;
    fflush(NULL);
    omp_init_lock(&_pull_lock);
    omp_init_lock(&_push_lock) ;

    read_network_info() ;

    int clause_size ;
    int max_clause = 1024 ;
    int* litbuf = (int*)malloc(sizeof(int)*max_clause) ;

    //create and listen on my socket
    int port = myaddr.port ;
    int listenfd, connfd = 0 ;
    int sockid ;
    struct sockaddr_in client_addr ;
    socklen_t ca_len ;

    struct sockaddr_in serv_addr ;
    memset(&serv_addr, '0', sizeof(serv_addr)) ;
    
    serv_addr.sin_family = AF_INET ;
    serv_addr.sin_port = htons(port) ;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY) ;

    sockid = socket(AF_INET, SOCK_STREAM, 0) ;
    if(sockid < 0) {
	printf("cant create socket!\n");
	return -1 ;
    }
    int errbind= bind(sockid, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) ;
    if(errbind < 0) {
	printf("cant bind \n");
	//is bind required? dont exit here
    }
    if(listen(sockid, 10) == -1) {
	printf("cant listen \n");
	return -1 ;
    }
    printf("accept now... \n");
    int s ;
    while(1) {
	s = accept(sockid, (struct sockaddr*) &client_addr, &ca_len);
	printf("waiting for recv \n") ;

	readn(s, &clause_size, sizeof(int)) ;
	clause_size = (int) ntohl(clause_size) ;
	//Now I know how much to read!
	readn(s, &litbuf, sizeof(int)*clause_size) ;
   
	process_new_clause(clause_size, litbuf) ;
	close(s) ;
    }
    //close socket
    //goto before the accept? after bind, definitely. keep listening as well? 

    return 0;
}

