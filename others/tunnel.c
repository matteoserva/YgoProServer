/*
 * $Id: tunnel.c,v 1.66 2001/07/30 15:10:19 coelho Exp $
 *
 *
 * COPYRIGHT
 *
 * (c) Fabien Coelho <fabien@coelho.net> 2000-2001
 *     sometimes on the web as http://www.coelho.net/
 *
 *
 * LICENSE
 * 
 * THIS PROGRAM IS DISTRIBUTED AS IS, WITHOUT ANY WARRANTY, UNDER
 * THE TERMS OF THE GNU GENERAL PUBLIC LICENSE (GNU-GPL).
 *
 * This means NO WARRANTY what so ever.
 *
 * Don't even expect the program to work for any particular purpose:
 * Maybe it is going to reboot your machine when run, or destroy your most 
 * precious data, or make you hairs becoming gray. Maybe all your friends 
 * are going to think you're a jerk because of the use of this program. 
 * So use at your own risks.
 *
 * You may modify the source code and distribute it as you want, provided
 * that the initial copyright is preserved and acknowledged and that your 
 * changes are clearly documented and signed in this section. Bug fixes, 
 * comments or additionnal features may be sent to the author who will
 * do whatever pleases him with them, including but not limited to, nothing.
 *
 * See http://www.gnu.org/ for more information.
 *
 *
 * DESCRIPTION
 * 
 * It does basic single process single thread TCP/IP tunnelling in C.
 * It is expected to be quite small and maybe efficient (?).
 * It is believed to be reasonnably coded with respect to MY standards;-)
 *
 * The process listens to a single ip/port.
 * Every incoming connexion is forwarded to a fixed destination.
 * SIGHUP (signal 1) may display the tunnel status.
 * No fork, no threads, the tunnel parallelism is managed thru select() calls.
 * There is a maximum number of simultaneous connexions.
 * There are many options that I needed for various purposes.
 * It does echo instead of tunnel if no destination is specified.
 *
 * 
 * DOCUMENTATION
 *
 * You hold it! It is this very source code!
 * See 'tunnel -h' for a list of available options.
 * See source code for the list of available compile-time options.
 * The comments in the source are not necessarily pertinent.
 *
 *
 * SEE ALSO
 * 
 * The 'bounce' program does the same (with forks and no options).
 * The 'Zedebee' program provides an compressed encrypted tcp/udp tunnel.
 * Many other programs or kernel/routeur settings to do a similar job.
 *
 *
 * COMPILATION
 *
 * This software is known to have run for me under 
 *   Linux/Intel, Solaris/Intel, Solaris/Sparc, SunOS/Sparc.
 *
 * Linux:   gcc -O2 -Wall                          tunnel.c -o tunnel
 * Solaris: gcc -O2 -Wall -lsocket -lnsl -DSOLARIS tunnel.c -o tunnel
 * SunOS:   gcc -O2 -Wall                -DSUNOS   tunnel.c -o tunnel
 * Anyway:  strip tunnel
 *
 */

/*********************************************************** C/UNIX INCLUDES */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>

/*************************************************************** PORTABILITY */

#if defined(SOLARIS)
#define herror perror
#endif /* SOLARIS */

#if defined(SUNOS)
/* getopt stuff */
extern char * optarg;
extern int optind, opterr;
/* quite partial compatibility... it won't work correctly. */
#define strtoul(a,b,c) atoi(a)
#define herror perror
typedef unsigned int ssize_t;
#endif /* SUNOS */

/******************************************************* COMPILATION OPTIONS */

/* whether SIGHUP can display the tunnel status. comment out to disable. */
#define SIGNAL_TO_STATUS 1

/* whether SNOOP option is compiled in. comment out to disable. */
#define MAY_SNOOP_TRAFFIC 1

/* whether to drop all output or allow some. comment out to disable.
 * if not set, both previous defines are also disabled.
 */
#define ALLOW_SOME_OUTPUT 1

/* whether to check an fd for writability alone,
 * or to put it globally in the full loop.
 * I don't know which one should be more efficient.
 * depends on select internals I guess?
 * on linux, even with a large max_connexions, it is better without.
 */
#undef CHECK_WRITE_ALONE

/* default connexion. */
#define LHOST "localhost" /* this really means 127.0.0.1, thus no network! */
#define LPORT "2023"
/* DHOST: <same as chosen LHOST> */
#define DPORT "23"        /* telnet port */

/*************************************************************** USEFUL DEFS */

#if !defined(ALLOW_SOME_OUTPUT)
/* maybe these function returns should be checked for errors. */
#define fputc(c,f) 
#define fputs(s,f) 
#define fprintf(f,a...) /* GCC only. */
#define fwrite(b,s,n,f)
#define perror(s) 
#define herror(s) 
#define gettimeofday(x,y)

/* definition coherency.
 */
#if defined(MAY_SNOOP_TRAFFIC)
#undef MAY_SNOOP_TRAFFIC
#endif /* MAY_SNOOP_TRAFFIC */

#if defined(SIGNAL_TO_STATUS)
#undef SIGNAL_TO_STATUS
#endif /* SIGNAL_TO_STATUS */
#endif /* ALLOW_SOME_OUTPUT */

/* very theoretical IP packet max size is 2^16==65536.
 * must be a multiple of sizeof(long) for the scramble loop.
 */
#define BUFFER_SIZE (1<<16)

/* maximum number of simultaneous connexions.
   could be a moved as an option with a default?
   is there a maximum? well, it depends on the operating system.
   there can be 1024 (0..1023) sockets for a process under my linux.
 */
#define DEFAULT_MAX_CONNEXIONS (512)

/* prefix when logging. */
#define LOG "[tunnel:%d] "

/* Left Circular Shift on a long by b bits. */
#define LCS(l,b) ((l<<b)|(((1<<b)-1)&(l>>(8*sizeof(long)-b))))

/* some convenient typedef that I like. */
typedef enum { false, true } boolean;
typedef char * string;

/******************************************************************* GLOBALS */

static boolean verbose = false;      /* -v/-q (verbose => log) */

#if (defined(ALLOW_SOME_OUTPUT))

static boolean silent = false;       /* -s to be very silent... */

#else

static boolean silent = true;

#endif /* ALLOW_SOME_OUTPUT */

static boolean debug  = false;       /* -d (debug => verbose) */
static boolean log    = false;       /* -l */

/* optionnal scramble with a simple xor. easy because stateless. */
static unsigned long scramble = 0x0; /* set with -p pass or -x long */

/* process id for logging purposes. */
static int pid;

/* echo instead of tunnel */
static boolean echo   = false;

#if defined(MAY_SNOOP_TRAFFIC)

/* Whether to snoop the INCOMING stream. 
 * This is intended as a way to log commands if the tunnel
 * is directed to a telnet port. This is not a sniffer, although
 * if used as described, it would collect passwords going thru.
 * Also useful for my HTTP practical exercise, to avoid the use
 * of tcpdump or snoop by the students.
 * Use OTP or SSH for something serious.
 * May insure that the snoop data are 'readable', by switching 
 * unprintable characteres to '.' with the -r option.
 */
static boolean snoop = false;        /* -L (snoop => log) */
static boolean printable = false;    /* -r */

/* size of snoop buffer. default is no buffer. */
static int snoopsize = 0;            /* -b size (=> snoop) */

#endif /* MAY_SNOOP_TRAFFIC */

/********************************************************* SOCKET CONNEXIONS */

static int serv_socket;              /* server socket which is listenned to. */

static struct sockaddr_in serv_addr; /* fixed server address */
static struct sockaddr_in dest_addr; /* fixed destination address */

/* describe a current connexion handled by the process. */
struct socket_connexion 
{
  boolean open;                   /* whether it is open/available */
  int index;                      /* index in array for log messages */
  
  /* connexion descriptor: a pair of socket and addr
   */
  struct sockaddr_in client_addr; /* to client side */
  int client;                     /* client socket */
  boolean client_r;               /* ready to read */
  boolean client_w;               /* ready to write */

  struct sockaddr_in dest_addr;   /* to destination side */
  int dest;                       /* dest socket */
  boolean dest_r;                 /* ready to read */
  boolean dest_w;                 /* ready to write */

  /* statistics for the connexion 
   */
  int requests;                   /* amount of requests client->dest. */
  int nreq;                       /* number of request paquets. */
  int responses;                  /* amount of responses dest->client. */
  int nres;                       /* number of response paquets. */
  
#if defined(MAY_SNOOP_TRAFFIC)
  
  /* snoop buffer for incoming traffic.
   */
  int sindex;                     /* index in the buffer. */
  char * snooped;                 /* buffer of snooped stuff. [snoopsize] */
  
#endif /* MAY_SNOOP_TRAFFIC */
  
};

/* array of current connexions. */
static struct socket_connexion * connexions;

/* maximum number of allowed connexions. */
static int max_connexions = DEFAULT_MAX_CONNEXIONS;

/* current number of connexions. */
static int number_of_connexions;

/* maximum index of open connexion. */
static int max_index_of_connexions;

/* various global statistics. */
static unsigned int total_number_of_connexions;
static unsigned int total_number_of_events;
static unsigned int total_number_of_bytes;

/* all connexions are closed on startup. zeros stats. */
static void initialize_connexions(void)
{
  int i;
  
  number_of_connexions = 0;
  total_number_of_connexions = 0;
  total_number_of_bytes = 0;
  total_number_of_events = 0;
  max_index_of_connexions = 0;

  connexions = (struct socket_connexion *) 
    malloc(max_connexions*sizeof(struct socket_connexion));
  
  if (!connexions) abort();

  for (i=0; i<max_connexions; i++)
  {
    connexions[i].open    = false;
    connexions[i].index   = i;

#if defined(MAY_SNOOP_TRAFFIC)

    connexions[i].sindex  = 0;
    connexions[i].snooped = snoopsize? (char *) malloc(snoopsize): NULL;

#endif /* MAY_SNOOP_TRAFFIC */

  }
}

static void set_max_index_of_connexions(int index)
{
  while (index>=0 && !connexions[index].open)
    index--;
  max_index_of_connexions = index+1;
}

/* returns a free chunk if any, or NULL */
static struct socket_connexion * available_connexion(void)
{
  int i;
  for (i=0; i<max_connexions; i++)
    if (!connexions[i].open)
      return &connexions[i];
  
  if (verbose) fputs("no more available connexions\n", stderr);

  return NULL;
}

/* shutdown the socket and check the result */
static void shutdown_socket(int socket)
{
  if (debug) 
    fprintf(stderr, "shuting down and closing socket %d\n", socket);

  if (shutdown(socket, 2) && verbose)
    perror("shutdown()");

  /* close the socket, otherwise it is kept until some timeout is reached. */
  if (close(socket) && verbose)
    perror("close()");
}

#if defined(MAY_SNOOP_TRAFFIC)

/* actually prints a buffer to stderr. */
static void print_snooped_buffer(char * buffer, int size, int src)
{
  fprintf(stderr, LOG "%d #%d: ", pid, src, size);
  fwrite(buffer, 1, size, stderr);
  fputc('\n', stderr);
}

/* flush snooped data to log and reset. */
static void flush_and_reset_snooped(struct socket_connexion * scp)
{
  if (snoop && scp->snooped) 
  {
    if (printable) /* switch non printable characters to '.' */
    {
      int i;
      for (i=0; i<scp->sindex; i++)
      {
	register char ci = scp->snooped[i];
	if ((ci<32 && ci!=10) || (ci > 126)) /* keep 10 + 32..126 */
	  scp->snooped[i] = '.';
      }
    }
    print_snooped_buffer(scp->snooped, scp->sindex, scp->client);
    scp->sindex = 0;
  }
}

/* keep or print snooped data */
static void append_snooped_data(struct socket_connexion * scp, 
				char * buffer, int size)
{
  if (scp->snooped)
  {
    /* pre-empty buffer is not large enough */
    if (scp->sindex && scp->sindex+size>=snoopsize)
      flush_and_reset_snooped(scp);
    
    /* put in snoop buffer */
    while (size)
    {
      while (scp->sindex<snoopsize && size)
	scp->snooped[scp->sindex++] = *buffer++, size--;
      
      if (scp->sindex==snoopsize)	
	flush_and_reset_snooped(scp);
    }
  }
  else /* no buffer */
  {
    print_snooped_buffer(buffer, size, scp->client);
  }
}

#endif /* MAY_SNOOP_TRAFFIC */

/* close the connexion. */
static void shutdown_connexion(struct socket_connexion * scp, string why)
{
  if (scp->open)
  {
    if (debug) 
      fprintf(stderr, "shuting down %d and %d\n", scp->client, scp->dest);

    if (scp->client!=-1) 
      shutdown_socket(scp->client);
      
#if defined(MAY_SNOOP_TRAFFIC)
      
    flush_and_reset_snooped(scp);
	
#endif /* MAY_SNOOP_TRAFFIC */

    if (scp->dest!=-1 && scp->dest!=scp->client) 
      shutdown_socket(scp->dest);

    if (log)
    {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      /* close could show collected statistics? */
      fprintf(stderr, 
	      LOG "#%d (%d,%d) %s 0x%08x:%d/0x%08x:%d at %ld.%06ld\n",
	      pid, scp->index, scp->client, scp->dest, why,
	      ntohl(scp->client_addr.sin_addr.s_addr),
	      ntohs(scp->client_addr.sin_port),
	      ntohl(scp->dest_addr.sin_addr.s_addr),
	      ntohs(scp->dest_addr.sin_port),
	      tv.tv_sec, tv.tv_usec);
    }

    scp->open   = false;
    scp->client = -1;
    scp->dest   = -1;
    number_of_connexions--; 
    if (max_index_of_connexions == scp->index+1)
      set_max_index_of_connexions(scp->index);
  }
}

  
/* connect the connexion or close the client
 */
static void open_connexion_or_shutdown(struct socket_connexion * scp)
{
  /* let's be optimistic... */
  scp->open      = true;
  scp->client_r  = false;
  scp->client_w  = false;
  scp->dest_r    = false;
  scp->dest_w    = false;
  scp->requests  = 0;
  scp->nreq      = 0;
  scp->responses = 0;
  scp->nres      = 0;

  number_of_connexions++;
  total_number_of_connexions++;
  if (scp->index >= max_index_of_connexions)
    max_index_of_connexions = scp->index+1;

  /* something to do if tunnel, nothing on echo. */
  if (scp->dest==-1)
  {
    if ((scp->dest = socket(AF_INET, SOCK_STREAM,  0))==-1)
    {
      if (verbose) perror("socket()");
      shutdown_connexion(scp, "error[socket]");
      return;
    }
    
    if (connect(scp->dest, 
		(struct sockaddr *) &scp->dest_addr, sizeof(struct sockaddr)))
    {
      if (verbose) perror("connect()");
      shutdown_connexion(scp, "error[connect]");
      return;
    }
    fprintf(stderr,"ciao%08x\n",ntohl(scp->client_addr.sin_addr.s_addr));
    
    uint16_t numB = 12;
    char buf[15];
    memcpy(buf,&numB,2);
    memcpy(&buf[2],"ipchange",8);
    memcpy(&buf[10],&scp->client_addr.sin_addr.s_addr,4);    
    
    write(scp->dest,buf,14);
  }

  if (log)
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    fprintf(stderr, LOG "#%d (%d,%d) open 0x%08x:%d/0x%08x:%d at %ld.%06ld\n",
	    pid, scp->index, scp->client, scp->dest, 
	    ntohl(scp->client_addr.sin_addr.s_addr),
	    ntohs(scp->client_addr.sin_port),
	    ntohl(scp->dest_addr.sin_addr.s_addr),
	    ntohs(scp->dest_addr.sin_port),
	    tv.tv_sec, tv.tv_usec);
  }
}

#if defined(CHECK_WRITE_ALONE)

/* quick check whether fd is available for writing */
static boolean can_write_now(int fd)
{
  struct timeval timeout;
  fd_set s;

  FD_ZERO(&s);
  FD_SET(fd, &s);

  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  
  if (select(fd+1, NULL, &s, NULL, &timeout)==-1)
    return false;
  
  return FD_ISSET(fd, &s)? true: false;
}

#endif /* CHECK_WRITE_ALONE */

/* transmit data if any
   src: source socket
   dst: destination socket to transmit data if any
   amount: pointer to byte statistics, or NULL
   n: pointer to event statistics, or NULL
   scp: pointer to socket_connexion if to be snooped.
 */
static boolean transmit(
    int src, 
    int dst, 
    int * amount, 
    int * n,
    struct socket_connexion * scp)
{
  static char buffer[BUFFER_SIZE]; /* yes, static, thus not on stack. */
  ssize_t rsize, wsize;
  
  if ((rsize = read(src, buffer, BUFFER_SIZE))==-1)
  {
    if (verbose) perror("read()");
    return false;
  }
  
  if (debug)
    fprintf(stderr, "transmit() %d -> %d, size=%d\n", src, dst, rsize);
  
  /* it may happen that an empty packet comes??? */
  if (rsize==0)
  {
    if (verbose) fputs("no data to read...\n", stderr);
    return false;
  }
  
#if defined(MAY_SNOOP_TRAFFIC)
  
  if (scp) append_snooped_data(scp, buffer, rsize);
  
#endif /* MAY_SNOOP_TRAFFIC */
    
  /* Optional scrambling (a simple 32 bits xor against a constant).
   * It is not intended as a cypher, but just to prevent basic dumps.
   */
  if (scramble) 
  {
    char * ptr = buffer;
    for (; ptr < buffer+rsize; ptr+=sizeof(long))
      * ((unsigned long *) ptr) ^= scramble;
  }
  
  if ((wsize = write(dst, buffer, rsize))==-1) /* should loop? */
  {
    if (verbose) perror("write()");
    return false;
  }
  
  if (wsize!=rsize)
  {
    if (verbose) 
      fprintf(stderr, "transmit sizes r=%d w=%d\n", rsize, wsize);
    return false;
  }
  
  /* update global and connexion statistics. */
  total_number_of_bytes += rsize;
  total_number_of_events++;
  if (amount) (*amount) += rsize;
  if (n) (*n)++;
  
  if (debug) fprintf(stderr, "transmit done\n");

  return true;
}

/* transmit data <-> for a connexion, and set fd_set as needed. */
static void transmit_connexion(
    struct socket_connexion * scp, 
    fd_set * ptoread,
    fd_set * ptowrite,
    fd_set * ptoexcept,
    int * n)
{
  int client = scp->client, dest = scp-> dest; /* saved as may be cleared */
  
  if (FD_ISSET(client, ptoread))
    scp->client_r = true;

  if (FD_ISSET(dest, ptowrite))
    scp->dest_w = true;

  if (!echo)
  {
    if (FD_ISSET(dest, ptoread))
      scp->dest_r = true;
    
    if (FD_ISSET(client, ptowrite))
      scp->client_w = true;
  }

  if (debug)
    fprintf(stderr, "IN transmit_connexion() of #%d (%d/%d%d,%d/%d%d)\n", 
	    scp->index, 
	    scp->client, scp->client_r, scp->client_w,
	    scp->dest, scp->dest_r, scp->dest_w);

#if defined(CHECK_WRITE_ALONE)

  if (scp->client_r && can_write_now(dest))
    scp->dest_w = true;

#endif /* CHECK_WRITE_ALONE */

  if (scp->open && scp->client_r && scp->dest_w)
  {
    if(!transmit(client, dest, &scp->requests, &scp->nreq, 

#if defined(MAY_SNOOP_TRAFFIC)

		 snoop? scp: NULL

#else

		 NULL

#endif /* MAY_SNOOP_TRAFFIC */

		 ))
      shutdown_connexion(scp, "close[client]");
    
    scp->client_r = false;
    scp->dest_w = false;
  }
     
  if (!echo)
  {

#if defined(CHECK_WRITE_ALONE)

    if (scp->dest_r && can_write_now(client))
      scp->client_w = true;

#endif /* CHECK_WRITE_ALONE */

    if (scp->open && scp->dest_r && scp->client_w)
    {
      if (!transmit(dest, client, &scp->responses, &scp->nres, NULL))
	shutdown_connexion(scp, "close[dest]");
      
      scp->dest_r = false;
      scp->client_w = false;
    }
  }

  /* update fd_set */
  if (scp->open)
  {
    /* still open */
    if (scp->client_r)
    {
      FD_CLR(client, ptoread);
      FD_SET(dest, ptowrite);
    }
    else
    {
      FD_SET(client, ptoread);
      FD_CLR(dest, ptowrite);
    }

    if (!echo)
    {
      if (scp->dest_r)
      {
	FD_CLR(dest, ptoread);
	FD_SET(client, ptowrite);
      }
      else
      {
	FD_SET(dest, ptoread);
	FD_CLR(client, ptowrite);
      }
    }

    FD_SET(dest, ptoexcept);
    FD_SET(client, ptoexcept);

    if (*n<client) *n = client;
    if (*n<dest) *n = dest;
  }
  else
  {
    /* the connexion was closed. nothing to wait for anymore. */
    FD_CLR(client, ptoread);
    FD_CLR(client, ptowrite);
    FD_CLR(dest, ptoread);
    FD_CLR(dest, ptowrite);
  }

  if (debug)
    fprintf(stderr, "OUT transmit_connexion() of #%d (%d/%d%d,%d/%d%d)\n", 
	    scp->index,
	    scp->client, scp->client_r, scp->client_w,
	    scp->dest, scp->dest_r, scp->dest_w);
}

/******************************************************************** STATUS */

#if defined(SIGNAL_TO_STATUS)

/* generate a report about current status and cumulated statistics to stderr */
static void info(int sig)
{
  struct timeval tv;
  int i;

  if (silent) return; /* we don't care now and latter. */

  gettimeofday(&tv, NULL);
  
  fprintf(stderr, 
	  LOG "status at %ld.%06ld: #con=%d/%d, #bytes=%d, #events=%d\n",
	  pid, tv.tv_sec, tv.tv_usec,
	  number_of_connexions, total_number_of_connexions,
	  total_number_of_bytes, total_number_of_events);
 
  for (i=0; i<max_index_of_connexions; i++)
  {
    struct socket_connexion * p = &connexions[i];
    if (p->open)
    {
      fprintf(stderr, LOG
	      "#%d (%d/%d%d,%d/%d%d):"
	      " 0x%08x:%d(e=%d,b=%d)/0x%08x:%d(e=%d,b=%d)\n",
	      pid, i, 
	      p->client, p->client_r, p->client_w,
	      p->dest, p->dest_r, p->dest_w, 
	      ntohl(p->client_addr.sin_addr.s_addr),
	      ntohs(p->client_addr.sin_port),
	      p->nreq, p->requests,
	      ntohl(p->dest_addr.sin_addr.s_addr),
	      ntohs(p->dest_addr.sin_port),
	      p->nres, p->responses);
    }
  }
  
  if (sig) signal(sig, info); /* rearm signal (linux, not as BSD) */
}

#endif /* SIGNAL_TO_STATUS */

/* is it useful? let say yes if buffering with snoop. */
static void down(int sig)
{
  int i;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  
  if (log) 
    fprintf(stderr, LOG "down (signal=%d) at %ld.%06ld\n", 
	    pid, sig, tv.tv_sec, tv.tv_usec);

#if defined(SIGNAL_TO_STATUS)

  info(0);

#endif /* SIGNAL_TO_STATUS */

  for (i=0; i<max_connexions; i++)
    shutdown_connexion(&connexions[i], "close[shutdown]");
  /* shutdown_socket(serv_socket); // no since no connexion. */
  
  fclose(stderr); /* the end. */
  
  /* if (sig) signal(sig, down); // rearm not needed, it's an exit. */
  exit(sig<<8);
}

/******************************************************************** SERVER */

#define INCOMING_QUEUE_SIZE 10

static int new_server(struct sockaddr * sa)
{
  int sn;
  int one = 1;
  
  if ((sn = socket(AF_INET, SOCK_STREAM,  0))==-1)
  {
    if (!silent) perror("server socket()");
    exit(1);
  }
  
  /* allow later reuse of the socket (without timeout) */
  if (setsockopt(sn, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)))
  {
    if (!silent) perror("setsockopt()");
    exit(2);
  }
  
  if (bind(sn, sa, sizeof(struct sockaddr)))
  {
    if (!silent) perror("bind()");
    exit(3);
  }
  
  if (listen(sn, INCOMING_QUEUE_SIZE))
  {
    if (!silent) perror("listen()");
    exit(4);
  }
  
  return sn;
}

static void dump_fd_set(string name, int n, fd_set * pfds)
{
  int i;
  fprintf(stderr, "%s: ", name);
  for (i=0; i<n; i++)
    if (FD_ISSET(i, pfds)) fprintf(stderr, "%d ", i);
  fprintf(stderr, "\n");
}

static void dump_fd_sets(string msg, int n, fd_set * r, fd_set * w, fd_set * e)
{
  fprintf(stderr, "fd set status '%s':\n", msg);
  dump_fd_set("read", n, r);
  dump_fd_set("write", n, w);
  dump_fd_set("except", n, e);
}

static void usage(string program, int exitcode)
{
  fprintf(stderr, 
	  "Usage: %s [-options...] lochost:port [dsthost:port]\n"
	  "  basic single process TCP/IP tunnel (or echo if no dest)\n"
	  "\t-d: debug mode (=> verbose)\n"
	  "\t-h: print this help\n"
	  "\t-l: log connexion activity to stderr\n"
	  
#if defined(MAY_SNOOP_TRAFFIC) /* snoop option help. */
	  
	  "\t-L: log activity stream to stderr (=> log)\n"
	  "\t-r: log with printable caracters only\n"
	  "\t-b size: buffer size for -L (default no buffer)\n"
	  
#endif /* MAY_SNOOP_TRAFFIC */
	  
	  "\t-q: quiet mode (not verbose, this is the default)\n"
	  "\t-s: silent (no output at all, even on errors)\n"
	  "\t-v: verbose mode (=> log)\n"
	  "\t-V: version\n"
	  "\t-M n: maximum number of simultaneous connexions (%d)\n"
	  "\t-x number: optional basic XOR scrambling\n"
	  "\t-p pass: idem, XOR constant based on pass\n"
	  "\t-m msg: insert message as a header to accepted connexions\n"
	  "\tlochost:port local host ip and tcp port to listen\n"
	  "\tdsthost:port tunnel destination host and tcp port\n"
	  "\tor from environment LHOST LPORT DHOST DPORT\n"
	  "\tor defaults: " LHOST ":" LPORT " <chosen local>:" DPORT "\n"
	  "\texample: %s -l localhost:2023 mir:23\n"
	  "\t  (send localhost:2023 packets to mir:23 and log)\n"
	  
#if defined(MAY_SNOOP_TRAFFIC)
	  
	  "\texample: %s -Lr localhost:1080 proxy:80\n"
	  "\t  (send localhost:1080 packets to proxy:80 and snoop)\n"
	  
#endif /* MAY_SNOOP_TRAFFIC */
	  
	  , program, DEFAULT_MAX_CONNEXIONS, program
	  
#if defined(MAY_SNOOP_TRAFFIC)
	  
	  , program
	  
#endif /* MAY_SNOOP_TRAFFIC */
	  
	  );
  exit(exitcode);
}

/********************************************************** LET'S DO THE JOB */

int main(int argc, char * argv[])
{
  int client_socket, len, i, code, n, opt;
  unsigned short int lport = 0, dport = 0; /* in_port_t */
  struct in_addr lhost, dhost;  /* in_addr_t */
  struct sockaddr_in client_addr;
  fd_set toread, towrite, toexcept;
  boolean okay = false;
  string lhosts = NULL, lports = NULL, dhosts = NULL, dports = NULL;
  string msg = NULL;
  
  /* option management. */
  boolean help = false;
  
  pid = (int) getpid();
  
  while ((opt=getopt(argc, argv, 

#if defined(MAY_SNOOP_TRAFFIC)

		     "b:Lr" /* snoop options. */

#endif /* MAY_SNOOP_TRAFFIC */

		     "dhlm:M:p:qsvVx:"))!=EOF)
  {
    switch (opt)
    {
    case 'd': debug = true; /* no break, debug => verbose */
    case 'v': verbose = true; /* no break, verbose => log */
    case 'l': log = true; break;
    case 's': silent = true; opterr = 0; break;
    case 'q': verbose = false; break;
      
#if defined(MAY_SNOOP_TRAFFIC)
      
      /* snoop-related option management. */
    case 'L': snoop = true; break;
    case 'r': printable = true; break;
    case 'b': snoopsize = strtoul(optarg, NULL, 0); break;
      
#endif /* MAY_SNOOP_TRAFFIC */

    case 'm': msg = strdup(optarg); break;
    case 'M': max_connexions = atoi(optarg); break;
    case 'x': scramble = strtoul(optarg, NULL, 0); break;
    case 'p': 
      /* 7 bits left circular shift and a XOR */
      while (*optarg) 
	scramble = LCS(scramble, 7)^(*optarg++);
      break;
    case 'h':
    case 'V':
    default: help = true; 
    }
  }
  
  /* these streams are not needed! */
  fclose(stdin); 
  fclose(stdout);

  /* insure option coherency. */
  if (silent) 
  {
    if (help) exit(7);
    verbose = false;
    log     = false;
    help    = false;
    debug   = false;
    fclose(stderr); /* one more socket to use! */
  }

#if defined(MAY_SNOOP_TRAFFIC)
  
  if (snoopsize) snoop = true;
  if (snoop) log = true;
  
#endif /* MAY_SNOOP_TRAFFIC */
  
  if (debug) verbose = true;
  if (verbose) log = true;
  
  /* arguments: lhost:lport dhost:dport or from environment
   * they should be checked?
   */
  if (argc==optind)
  {
    okay   = true;

    /* configure from environment, with defaults. */
    lhosts = getenv("LHOST");
    lports = getenv("LPORT");
    dhosts = getenv("DHOST");
    dports = getenv("DPORT");
  }
  
  if (argc-optind>=1)
  {
    okay = true;
    echo = true;
    
    /* local */
    lhosts = argv[optind];
    lports = strchr(lhosts, ':');
    if (lports) *lports++ = '\0';
  }
  
  if (argc-optind==2)
  {
    okay = true;
    echo = false;

    /* destination */
    dhosts = argv[optind+1];
    dports = strchr(dhosts, ':');
    if (dports) *dports++ = '\0';
  }

  /* set default values (as string for homogeneity) if needed.
   */
  if (!lhosts || !*lhosts) lhosts = LHOST;
  if (!lports || !*lports) lports = LPORT;
  if (!dhosts || !*dhosts) dhosts = lhosts;
  if (!dports || !*dports) dports = DPORT;
  
  if (okay)
  {
    struct hostent * he = gethostbyname(lhosts);
    if (!he) 
    {
      if (!silent) herror("gethostbyname()");
      exit(5);
    }
    lhost.s_addr = *((unsigned int *)he->h_addr_list[0]);
    lport = atoi(lports); /* check? */
    
    he = gethostbyname(dhosts);
    if (!he) 
    {
      if (!silent) herror("gethostbyname()");
      exit(6);
    }
    dhost.s_addr = *((unsigned int *)he->h_addr_list[0]);
    dport = atoi(dports); /* check? */
  }
  else /* no or bad configuration. */
  {
    if (silent) exit(8);
    help = true;
  }
  
  if (verbose || help)
    fputs("TCP/IP tunnel $Revision: 1.66 $\n", stderr);
  
  if (help) usage(argv[0], 0);
  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(lport);
  serv_addr.sin_addr = lhost;
  
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(dport);
  dest_addr.sin_addr = dhost;
  
  if (log) 
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (echo)
      fprintf(stderr, 
	      LOG "start echo 0x%08x:%d x=0x%08lx at %ld.%06ld\n",
	      pid,
	      ntohl(serv_addr.sin_addr.s_addr), 
	      ntohs(serv_addr.sin_port),
	      scramble, tv.tv_sec, tv.tv_usec);
    else
      fprintf(stderr, 
	      LOG "start tunnel 0x%08x:%d/0x%08x:%d x=0x%08lx at %ld.%06ld\n",
	      pid,
	      ntohl(serv_addr.sin_addr.s_addr), 
	      ntohs(serv_addr.sin_port),
	      ntohl(dest_addr.sin_addr.s_addr), 
	      ntohs(dest_addr.sin_port),
	      scramble, tv.tv_sec, tv.tv_usec);
  }
  
#if defined(SIGNAL_TO_STATUS)
  
  signal(SIGHUP, info);
  
#endif /* SIGNAL_TO_STATUS */
  
  signal(SIGINT,  down);
  signal(SIGQUIT, down);
  signal(SIGABRT, down);
  signal(SIGTERM, down);
  
  initialize_connexions();
  
  serv_socket = new_server((struct sockaddr *) &serv_addr);
  
  /* initial file descriptor set */
  FD_ZERO(&toread);
  FD_ZERO(&towrite);
  FD_ZERO(&toexcept);
  FD_SET(serv_socket, &toread);
  FD_SET(serv_socket, &toexcept);
  n = serv_socket+1;
  
  while ((code=select(n, &toread, &towrite, &toexcept, NULL))!=-1 ||
	 (code==-1 && errno==EINTR) /* allow signals */ ||
	 true) /* on select errors, let us go on anyway? */
  {
    if ((code==-1 && errno!=EINTR) && verbose)
      perror("select()");

    if (code==-1 && errno==EINTR)
    {
      if (debug) fprintf(stderr, "zeroing sets after select() interrupt\n");
      FD_ZERO(&toread);
      FD_ZERO(&towrite);
      FD_ZERO(&toexcept);
    }

    if (debug)
    {
      dump_fd_sets("after select", n, &toread, &towrite, &toexcept);
      fprintf(stderr, "select code=%d\n", code);
    }
    
    if (code!=-1 && FD_ISSET(serv_socket, &toread))
    {
      /* it is a new connexion. */
      total_number_of_events++;
      len = sizeof(struct sockaddr);
      
      client_socket = 
	accept(serv_socket, (struct sockaddr*) &client_addr, &len);
      
      if (client_socket==-1 && verbose)
	perror("accept()"); /* let us ignore... ??? */
      
      if (client_socket>0)
      {
	struct socket_connexion * scp = available_connexion();

	if (scp) 
	{
	  /* should be checked? */
	  if (msg) write(client_socket, msg, strlen(msg));
	  
	  scp->client      = client_socket;
	  scp->client_addr = client_addr;

	  if (echo)
	  {
	    /* back to client */
	    scp->dest_addr = client_addr;
	    scp->dest      = client_socket;
	  }
	  else
	  {
	    /* fixed destination */
	    scp->dest_addr = dest_addr;
	    scp->dest      = -1;
	  }

	  open_connexion_or_shutdown(scp);
	}
	else
	{
	  if (log)
	    fprintf(stderr, LOG "%d client refused, max #connect reached\n", 
		    pid, client_socket);
	  shutdown_socket(client_socket);
	}
      }
    }
    
    FD_ZERO(&toexcept);
    FD_SET(serv_socket, &toexcept);
    FD_SET(serv_socket, &toread);
    n = serv_socket;

    /* transmit if needed. maybe the list of open connexions could be kept? */
    for (i=0; i<max_index_of_connexions; i++)
      if (connexions[i].open)
	transmit_connexion(&connexions[i], &toread, &towrite, &toexcept, &n);

    n++; /* select() expects the largest socket number + 1 */

    if (debug) dump_fd_sets("before select", n, &toread, &towrite, &toexcept);
  }
  
  /* if (!silent) perror("select()"); */
  down(0);
  return 9; /* never reached. */
}
