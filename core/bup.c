#define _FILE_OFFSET_BITS 64 

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <endian.h>

/********************************************************************************/
#define DEBUG 0

#define debug_print(...) \
  do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)

#define log_err(M, ...) \
  do{ fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, strerror(errno), ##__VA_ARGS__); } while(0); 

#define blog debug_print

/********************************************************************************/
int fd ; //fd for the backup file...
char* fname ;
unsigned int MAX_BATCH_SIZE = 1024 ; //is 1024, double it to 2048
unsigned int PAGE_SIZE = 4096 ;
off_t skel_offset = 0 ; //write to this later! 
unsigned long* pfn_types = NULL ;
unsigned long* pfn_types_batch = NULL ;
char* thepage = NULL; 
char* header_page = NULL ;
size_t numpages ;
#define XEN_DOMCTL_PFINFO_LTAB_MASK (0xfU<<28)

#define XEN_DOMCTL_PFINFO_XTAB    (0xfU<<28) /* invalid page */
char* LinuxGuestRecord = "LinuxGuestRecord" ;


off_t endofpages = 0 ;
int bsim = 0 ; 
/********************************************************************************/

/** FORMAT:
    LinuxGuestRecord
    vmconfig size (integer)
    Sexp
    ----- Above slurped by Xendcheckpoint ------------------
    ----- XC save AND XC restore takes control HERE! --------
    skel start offset (absolute offset in the file!)
    p2m size
    0 padding
    -------- PAGES START--------------

    --------- PAGES END--------------
    skel magic marker
    skel size
    Skeleton state
      skel p2m
    pfn_types 
    --------END-------------
*/

/* Read 'n' bytes from 'fd' into 'buf', restarting after partial
   reads or interruptions by a signal handlers */

ssize_t readn(int fd, void *buffer, size_t n)
{
    ssize_t numRead;                    /* # of bytes fetched by last read() */
    size_t totRead;                     /* Total # of bytes read so far */
    char *buf;

    buf = buffer;                       /* No pointer arithmetic on "void *" */
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

    buf = buffer;                       /* No pointer arithmetic on "void *" */
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

ssize_t write_log(int fd, const void* buffer, size_t n, char* field)
{
  off_t foff = lseek(fd, 0, SEEK_CUR) ;
  blog("@ %zu -- %zu :  %s \n", foff, n, field); 
  return writen(fd, buffer, n); 
}

/********************************************************************************/

/* only call after reading p2m size (numpages). Writes header page, sexp, p2m_size and hole for seek offset!  */
int init_buffers(char* hdr, char* sexp_buffer, int sexp_len, unsigned long skel_offset_hole, unsigned long numpages)
{
  pfn_types_batch = calloc(MAX_BATCH_SIZE+1, sizeof(unsigned long)) ;
  pfn_types = calloc(numpages+2, sizeof(unsigned long)); 
  thepage = calloc(PAGE_SIZE,sizeof(char)) ;
 
  header_page = calloc(PAGE_SIZE, sizeof(char)) ; //for alignment!
  //  memcpy(skel_offset_page, &skel_offset, sizeof(unsigned int)) ;

  fd = open(fname, O_CREAT|O_RDWR|O_TRUNC) ; //can replace by creat(O_WRONLY)
  if(fd==-1) {
    blog("error opening file \n") ;
    return -1 ;
  }
  if(bsim) {
    return 0 ;
  }
  write_log(fd, header_page, PAGE_SIZE,"header page zero") ; //first thing to be written.
  lseek(fd, 0, SEEK_SET) ; 
  write_log(fd, hdr, 16, "LinuxGuestRecord") ;
  write_log(fd, &sexp_len, sizeof(int),"length SEXP") ;
  write_log(fd, sexp_buffer, sexp_len, "SEXP");
  /* save this offset for later use! */
  skel_offset = lseek(fd, 0 , SEEK_CUR) ;
  write_log(fd, &skel_offset_hole, sizeof(unsigned long),"SKEL_OFFSET_HOLE") ;
  write_log(fd, &numpages, sizeof(unsigned long), "NUM PAGES P2M SIZE") ;
  return  0 ;
}

/********************************************************************************/

int write_page(char* page, unsigned long  pfn_type)
{
  unsigned long pfn_long = pfn_type & ~XEN_DOMCTL_PFINFO_LTAB_MASK ;
  off_t offset =(off_t)(pfn_long*PAGE_SIZE + PAGE_SIZE) ; //one full page for the header
  //write or mmap page into the pfn offset in the file and done!!!
  //  blog("+++ writing page %lu  to offset %lu \n", pfn_long, offset);
  if(offset > endofpages) {
    endofpages = offset ;
  }
  lseek(fd, offset, SEEK_SET) ;
  writen(fd, page, PAGE_SIZE) ;
  return 0 ;
}

/********************************************************************************/

unsigned long csum_page(void* page)
{
    int i;
    unsigned long *p = page;
    unsigned long long sum=0;

    for ( i = 0; i < (PAGE_SIZE/sizeof(unsigned long)); i++ )
        sum += p[i];

    return sum ^ (sum>>32);
}



int handle_batch(int sockid, unsigned int batch)
{
  //read the pfn_types array, for .... saving later?
  int real_send = 1 ;
  int count ;
  int j = 0 ;
  int i = 0; 
  unsigned int pages_to_expect = batch ;
  
  blog("batch of size %u\n",batch); 
  if (batch > MAX_BATCH_SIZE) {
    blog("batch size too large... realloc? \n");
    return -1 ;
  }
  count =  readn(sockid, pfn_types_batch, batch*sizeof(unsigned long)) ;
  if(count != batch*sizeof(unsigned long)) {
    blog("wrong number of pfn types recvd %d count out of %d \n",count, batch) ;
    return -1 ;
  }
  for(j = 0; j < batch; j++) {
    unsigned long pfn = pfn_types_batch[j] & ~XEN_DOMCTL_PFINFO_LTAB_MASK ;
    unsigned long type = pfn_types_batch[j] & XEN_DOMCTL_PFINFO_LTAB_MASK ;
    blog("%lu, ",pfn_types_batch[j]) ;
    if(type != 0L) {
      //blog("Strange, type is not zero of page %lu %lu \n",pfn,type);
    }
    if(type == XEN_DOMCTL_PFINFO_XTAB) {
      pages_to_expect-- ;
    }
    if (pfn > numpages) {
      blog("*** pfn larger than p2m size found!! %lu \n",pfn);
      return -1 ;
    }
    pfn_types[pfn] = pfn_types_batch[j] ;
  }
  blog("\n");
  blog("^^^^^^ EXPECTING %d pages of %d \n", pages_to_expect, batch) ;
  /* could replace this by a parallel operation, but then we might block on reading a full batch of pages?*/
  if(!real_send) 
    return 0;
 
  unsigned long ptype ; 
  while(i < batch) {
    ptype = pfn_types_batch[j] & XEN_DOMCTL_PFINFO_LTAB_MASK ;
    if(ptype == XEN_DOMCTL_PFINFO_XTAB) {
      //this is NOT
      i++ ;
      continue ;
    }
    int r = readn(sockid, thepage, PAGE_SIZE) ;
    if(r != PAGE_SIZE) {
      blog("error reading pages %d\n", r);
      return -1 ;
    }

    if(i==0 || i == pages_to_expect-1) {
      blog("%d-th PAGE %lu CKSUM : %lu \n",i, (pfn_types_batch[i] & ~XEN_DOMCTL_PFINFO_LTAB_MASK) , csum_page(thepage)) ;
    }
    // FIX THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    /* Write to correct page offset */
      if(write_page(thepage, pfn_types_batch[i])) {
       blog("exiting because cant write page %lu \n", pfn_types_batch[i]);
      }    
    i++ ;
  }
  blog("Saved batch \n") ;
  return 0;
}

/********************************************************************************/

int update_skel_offset(int fd, off_t offval)
{
  off_t towrite = skel_offset ;
  blog("offt %d vs ulong %d \n",sizeof(off_t), sizeof(unsigned long));

  unsigned long val = (unsigned long) offval ;
  blog("offval is %lu .... val is %lu \n",offval,  val);
  off_t targ = lseek(fd, towrite, SEEK_SET) ;
  blog("seeked to %ul \n",targ );
  writen(fd, &val, sizeof(unsigned long)) ; 
  blog("WRITTEN %lu into this offset : %lu \n",val, towrite) ;
  close(fd) ;
  return 0;
}


/* skeleton buffer,size, and the pfn_types array after that? */
int dump_image_tail(char* skel, unsigned long skel_size)
{
  /* At this point the warning has been received. Fadvise here!, although SSDs might blunt this */
  posix_fadvise(fd, 0, 0, POSIX_FADV_WILLNEED|POSIX_FADV_NOREUSE) ; 
  /* Haha, linux will probably just ignore this */

  //lseek to the file? append? 
  off_t pageend = lseek(fd, 0, SEEK_CUR) ;
  blog("$$$ end of file is at %lu, but calculated to be %lu \n", pageend, (numpages+1)*PAGE_SIZE);

  write_log(fd, skel, skel_size, "SKELETON BUFFER") ;

  blog("After writing skel, offset is %lu \n", lseek(fd, 0, SEEK_CUR));
  off_t rew = lseek(fd, pageend, SEEK_SET) ;
  blog("seeking to offset %lu, where skel is supposed to reside!\n", rew) ;
  unsigned long s1,s2 ;
  readn(fd, &s1, sizeof(unsigned long)) ; blog("marker : %lu \n",s1) ;
  readn(fd, &s2, sizeof(unsigned long)) ; blog("skel size : %lu \n",s2) ;
  blog("reading skel back! \n" ) ;
  //  write_log(fd, pfn_types, (numpages)*sizeof(unsigned long), "PFN TYPES") ; //extra zero???? 
  blog("image tail dumped of size %lu\n", skel_size);
  
  return 0 ; //done!!
}

/********************************************************************************/

int open_socket_stuff(int port) 
{
  int listenfd, connfd = 0 ;
  struct sockaddr_in serv_addr ;
  memset(&serv_addr, '0', sizeof(serv_addr)) ;

  serv_addr.sin_family = AF_INET ;
  serv_addr.sin_port = htons(port) ;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY) ;
  int sockid ;
  sockid = socket(AF_INET, SOCK_STREAM, 0) ;
  if(sockid < 0) {
    blog("cant create socket!\n");
    return -1 ;
  }
  int errbind= bind(sockid, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) ;
  if(errbind < 0) {
    blog("cant bind \n");
    //is bind required? dont exit here
  }
  if(listen(sockid, 10) == -1) {
    blog("cant listen \n");
    return -1 ;
  }
  struct sockaddr_in client_addr ;
  socklen_t ca_len ;
  blog("accept now... \n");
  int s = accept(sockid, (struct sockaddr*) &client_addr, &ca_len);
  char c ;
  int count ;
  blog("waiting for recv \n") ;

  unsigned int batch ;
  unsigned long skel_size ;
  unsigned long p2m_size_raw = 0 ;
  unsigned int skel_offset_hole = 0; 
  char* skel_buffer = NULL ; 
  char* hdr = malloc(20*sizeof(char)) ;
  int sexp_len = 0 ;
  char* sexp_buffer = NULL ;
  char* tmp = malloc(1*sizeof(char)) ;
  unsigned long skel_magic_marker = 0 ;
  int skfd ;
  off_t skel_start_location = 0 ;
  blog("int size is %d \n",sizeof(int)) ;
  bsim = 0 ;
  if(bsim==1) {
    goto p2m_size;
  }
 read_header: 
  readn(s, hdr, 16) ;
  blog("HDR : %s \n", hdr) ;

  if(!strcmp(hdr, LinuxGuestRecord)) {
    blog("headers dont match! \n");
  }
  
 read_sexp:
  readn(s, &sexp_len, sizeof(int)) ;
  sexp_len = (int) ntohl(sexp_len) ;
  blog("SEXP LEN %d, %x \n",sexp_len, sexp_len) ;
  
  if(sexp_len <= 0 || sexp_len >3000 ) {
    blog("sexp len is invalid %dx\n", sexp_len);
    sexp_len = 1325 ;
  }
  sexp_buffer = malloc(sexp_len) ;
  if(!sexp_buffer) {
    blog("maloc failed for sexp buffer \n") ;
  }
  readn(s, sexp_buffer, sexp_len) ;
  blog("SEXP BUFFER : %s \n", sexp_buffer) ;

 seek_offset :
  count = readn(s, &skel_offset_hole, sizeof(unsigned long)) ;
  blog("seek offset %lu\n", skel_offset_hole);
  if(count <= 0) {
    blog("whooops , neg count in seek_offset \n");
    return -1 ;
  }

 p2m_size:
  count = readn(s, &p2m_size_raw, sizeof(unsigned long));
  blog("p2m size is %lu \n", p2m_size_raw) ;
  size_t p2m_size = (size_t) p2m_size_raw ;
  numpages = p2m_size ;

 read_page_batches:
  init_buffers(hdr, sexp_buffer, sexp_len,  skel_offset_hole, numpages) ;
  while(1) {
    count = readn(s, &batch, sizeof(unsigned int)) ;
    if(count <= 0) {
      blog("whooops , cant read batch size! \n");
      return -1 ;
    }
    if(batch==0) {
      blog("end of the batches (size zero) \n") ;
      continue ;
      //      goto read_skeleton; 
      //%%%%%%%%%%%%%%%%%%%%%% FIX THIS.
    }
    if(batch > MAX_BATCH_SIZE) {
      continue ;
    }
    if(handle_batch(s, batch)) {
      blog("error handling batch. continue??? \n");
      goto out ;
    }    
  } //end while(1)
  if(bsim) {
    goto out ;
  }
 read_skeleton :
  skel_start_location = lseek(fd, 0, SEEK_CUR) ;
  blog("skel starts at %lu \n",skel_start_location) ;
  skfd = open("skel.img", O_CREAT|O_RDWR|O_TRUNC) ;
  //  while(1) {
  
  readn(s, &skel_magic_marker, sizeof(unsigned long)) ;
  blog("skel magic marker %lu \n", skel_magic_marker) ; 
  
  readn(s, &skel_size, sizeof(unsigned long)) ;
  blog("skel size is %ul \n",skel_size);
  
  skel_buffer = malloc(skel_size) ;
  if(skel_buffer==NULL) {
    blog("oops, cant allocate memory for skel state of %lu \n",skel_size) ;
    return -1 ;
  }
  memcpy(skel_buffer, &skel_magic_marker, sizeof(unsigned long)) ;
  memcpy(skel_buffer+sizeof(unsigned long), &skel_magic_marker, sizeof(unsigned long)) ;
  
  count = readn(s, skel_buffer+(2*sizeof(unsigned long)), skel_size) ; //about 50K, should be OK
  dump_image_tail(skel_buffer, skel_size) ; //also write the pfn_types buffer here?
  update_skel_offset(fd, skel_start_location);
  
 out:
  blog("exiting!\n") ;
  return 0 ;
}
 
/********************************************************************************/
/* 
 * usage: ./program port filename
 */

int main(int argc, char** argv)
{
  if(argc < 2) {
    printf("wrong args specified \n");
    exit(-1);
  }
  char* port = argv[1] ;
  char* pfname = argv[2] ;
  int portnum = atoi(port) ;
  fname = pfname ;
  open_socket_stuff(portnum) ;

  return 0 ;
}

