/***********************************************/
/* JUMP DSM: DSM using Migrating-Home Protocol */
/***********************************************/
/* Modified from JIAJIA 1.1: Released Aug 2001 */
/***********************************************/

/***********************************************************************
 *                                                                     *
 *   The JIAJIA Software Distributed Shared Memory System              * 
 *                                                                     *
 *   Copyright (C) 1997 the Center of High Performance Computing       * 
 *   of Institute of Computing Technology, Chinese Academy of          *      
 *   Sciences.  All rights reserved.                                   *
 *                                                                     *
 *   Permission to use, copy, modify and distribute this software      *
 *   is hereby granted provided that (1) source code retains these     *
 *   copyright, permission, and disclaimer notices, and (2) redistri-  *
 *   butions including binaries reproduce the notices in supporting    *
 *   documentation, and (3) all advertising materials mentioning       *
 *   features or use of this software display the following            *
 *   acknowledgement: ``This product includes software developed by    *
 *   the Center of High Performance Computing, Institute of Computing  *
 *   Technology, Chinese Academy of Sciences."                         *
 *                                                                     *
 *   This program is distributed in the hope that it will be useful,   *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              * 
 *                                                                     *
 *   Center of High Performance Computing requests users of this       *
 *   software to return to dsm@water.chpc.ict.ac.cn any                * 
 *   improvements that they make and grant CHPC redistribution rights. *    
 *                                                                     * 
 *           Author: Weiwu Hu, Weisong Shi, Zhimin Tang                *      
 * =================================================================== *
 *   This software is ported to SP2 by                                 *
 *                                                                     *
 *         M. Rasit Eskicioglu                                         *
 *         University of Alberta                                       *
 *         Dept. of Computing Science                                  *
 *         Edmonton, Alberta T6G 2H1 CANADA                            *
 * =================================================================== *
 * $Log: tools.c,v $
 * Revision 1.1  1998/03/06 05:20:14  dsm
 * add get_usecs()
 *
 * Revision 1.2  1998/03/05 07:44:17  rasit
 * New function: get_usecs()
 *
 * Revision 1.1  1998/02/20 20:47:38  rasit
 * Added perror() call to assert0()
 * Some cleanup
 *
 * Revision 1.0  1998/02/19 18:34:02  rasit
 * Initial revision
 *
 **********************************************************************/

static char rcsid_tools_c[] = "$Id: tools.c,v 1.1 1998/03/06 05:20:14 dsm Exp $";

#ifndef NULL_LIB
#include "global.h"
#include "init.h"
#include "mem.h"
#include "syn.h"
#include "comm.h"

sigset_t set, prevset;

extern void asendmsg(jia_msg_t *msg);

void assert0(int, char *);
void assert(int, char *);
void jiaexitserver(jia_msg_t *req);
jia_msg_t *newmsg();
void freemsg(jia_msg_t *msg);
void appendmsg(jia_msg_t *msg, unsigned char *str, int len);
void printmsg(jia_msg_t *msg,int right);
void printstack(int ptr);
unsigned long jia_current_time();
float jia_clock();
void jiasleep(unsigned long);
void disable_sigio();         
void disable_sigalrm();         
void enable_sigalrm();       
void enable_sigio();       
void freewtntspace(wtnt_t *ptr);
wtnt_t *newwtnt();
address_t newtwin();
void freetwin(address_t twin);
void emptyprintf();

extern int alarmdisabled;
extern int jia_pid;
extern int hostc;
extern jiastack_t lockstack[Maxstacksize];
extern int totalhome;
extern host_t hosts[Maxhosts];
extern char argv0[Wordsize];
extern int sigflag;

char errstr[Linesize];
jia_msg_t msgarray[Maxmsgs];
volatile int msgbusy[Maxmsgs];
int msgcnt;
jia_msg_t assertmsg;


/*-----------------------------------------------------------*/
void assert0(int cond, char *amsg)
{ 
  if (!cond) {
   fprintf(stderr,"Assert0 error from host %d --- %s\n", jia_pid, amsg);
   perror("Unix Error");
   fflush(stderr); fflush(stdout);
   exit(-1);
  }
}


/*-----------------------------------------------------------*/
void assert(int cond, char *amsg)
{
  int hosti;

  if (!cond) {
    assertmsg.op = JIAEXIT;
    assertmsg.frompid = jia_pid; 
    memcpy(assertmsg.data, amsg, strlen(amsg));
    assertmsg.data[strlen(amsg)] = '\0';
    assertmsg.size = strlen(amsg) + 1;
    for (hosti = 0; hosti < hostc; hosti++)
      if (hosti != jia_pid) {
       assertmsg.topid = hosti;
       asendmsg(&assertmsg);
      }
    assertmsg.topid = jia_pid;
    asendmsg(&assertmsg);
  } 
}

/*-----------------------------------------------------------*/
void jiaexitserver(jia_msg_t *req)
{
  printf("Assert error from host %d --- %s\n",
	  req->frompid, (char*)req->data);
  fflush(stderr); fflush(stdout);
  exit(-1);
}

/*-----------------------------------------------------------*/
address_t newtwin()
{
  address_t twin;
  int allocsize;

  allocsize = Pagesize;
  twin = (address_t)valloc((size_t)allocsize);

  assert((twin != (address_t)NULL), "Cannot allocate twin space!");
  return(twin);
}

/*-----------------------------------------------------------*/
void freetwin(address_t twin)
{
  free(twin);
}

/*-----------------------------------------------------------*/
jia_msg_t *newmsg()
{
  int i,j;

  msgcnt++;
  for (i = 0; (i < Maxmsgs) && (msgbusy[i] != 0); i++);
  assert0((i < Maxmsgs), "Cannot allocate message space!");
  msgbusy[i] = 1;

#ifdef JIA_DEBUG
  for (j = 0; j < Maxmsgs; j++) printf("%d ", msgbusy[j]);
  printf("  msgcnt = %d\n", msgcnt);
#endif

 return(&(msgarray[i]));
}


/*-----------------------------------------------------------*/
void freemsg(jia_msg_t *msg)
{
  int i;
  msgbusy[msg->index] = 0;
  msgcnt--;
}

/*-----------------------------------------------------------*/
void appendmsg(jia_msg_t *msg, unsigned char *str, int len)
{
  assert(((msg->size + len) <= Maxmsgsize), "Message too large");
  memcpy(msg->data + msg->size, str, len);
  msg->size += len;
}

wtnt_t *newwtnt()
{
  wtnt_t *wnptr;

#ifdef SOLARIS
  wnptr = memalign((size_t)Pagesize, (size_t)sizeof(wtnt_t));
#else  /* SOLARIS */
  wnptr = valloc((size_t)sizeof(wtnt_t));
#endif /* SOLARIS */

  assert((wnptr != WNULL), "Cannot allocate space for write notices!");
  wnptr->more = WNULL;
  wnptr->wtntc = 0;
  return(wnptr);
}

/*-----------------------------------------------------------*/
void freewtntspace(wtnt_t *ptr)
{
  wtnt_t *last, *wtntp;

  wtntp = ptr->more;
  while(wtntp != WNULL) {
    last = wtntp;
    wtntp = wtntp->more;
    free(last);
  }
  ptr->wtntc = 0;
  ptr->more = WNULL;
}

/*-----------------------------------------------------------*/
void printmsg(jia_msg_t *msg, int right)
{ 
  SPACE(right); printf("********Print message********\n");
  SPACE(right); switch (msg->op) {
    case DIFF:      printf("msg.op      = DIFF     \n"); break;
    case DIFFGRANT: printf("msg.op      = DIFFGRANT\n"); break;
    case GETP:      printf("msg.op      = GETP     \n"); break;
    case GETPGRANT: printf("msg.op      = GETPGRANT\n"); break;
    case ACQ:       printf("msg.op      = ACQ      \n"); break;
    case ACQGRANT:  printf("msg.op      = ACQGRANT \n"); break;
    case INVLD:     printf("msg.op      = INVLD    \n"); break;
    case BARR:      printf("msg.op      = BARR     \n"); break;
    case BARRGRANT: printf("msg.op      = BARRGRANT\n"); break;
    case REL:       printf("msg.op      = REL      \n"); break;
    case WTNT:      printf("msg.op      = WTNT     \n"); break;
    case STAT:      printf("msg.op      = STAT     \n"); break;
    case STATGRANT: printf("msg.op      = STATGRANT\n"); break;
    case JIAEXIT:   printf("msg.op      = JIAEXIT  \n"); break;
    default:        printf("msg.op      = %d       \n",msg->op); break;
  } 
  SPACE(right); printf("msg.frompid = %d\n",msg->frompid);
  SPACE(right); printf("msg.topid   = %d\n",msg->topid);
  SPACE(right); printf("msg.syn     = %d\n",msg->syn);
  SPACE(right); printf("msg.seqno   = %d\n",msg->seqno);
  SPACE(right); printf("msg.index   = %d\n",msg->index);
  SPACE(right); printf("msg.size    = %d\n",msg->size);
  SPACE(right); printf("msg.data    = 0x%8x\n",stol(msg->data));
  SPACE(right); printf("msg.data    = 0x%8x\n",stol(msg->data+4));
  SPACE(right); printf("msg.data    = 0x%8x\n",stol(msg->data+8));
}

/*-----------------------------------------------------------*/
/* Following programs are used by Shi. 9.10 */

unsigned long start_sec = 0; 
unsigned long start_time_sec = 0; 
unsigned long start_time_usec = 0; 
unsigned long start_msec = 0;  

/*-----------------------------------------------------------*/
unsigned long jia_current_time()
{
  struct timeval tp;

  /* Return the time, in milliseconds, elapsed after the first call
   * to this routine.
   */  
  gettimeofday(&tp, NULL);
  if (!start_sec) {
    start_sec = tp.tv_sec;
    start_msec = (unsigned long) (tp.tv_usec / 1000);
  }
  return (1000 * (tp.tv_sec - start_sec) 
	 + (tp.tv_usec / 1000 - start_msec));
}
 
/*-----------------------------------------------------------*/
float jia_clock()
{
  struct timeval val;
  gettimeofday(&val, NULL);
  if (!start_time_sec) {
    start_time_sec = val.tv_sec;
    start_time_usec = val.tv_usec;
  }
  return (float)(((val.tv_sec * 1000000.0 + val.tv_usec) -
	 (start_time_sec * 1000000.0 - start_time_usec)) / 1000000.0);
}

/*-----------------------------------------------------------*/
void disable_sigio() 
{
  sigset_t oldset;

  sigemptyset(&set);
  sigaddset(&set, SIGIO);
  sigaddset(&set, SIGALRM);
  sigprocmask(SIG_BLOCK, &set, &oldset);
  sigflag = 0;
}

void disable_sigalrm()
{
  sigset_t oldset;

  sigemptyset(&set);
  sigaddset(&set, SIGALRM);
  sigprocmask(SIG_BLOCK, &set, &oldset);
  alarmdisabled = 1;
}

/*
int alarmon() {
   return alarmdisabled;
}
*/

/*-----------------------------------------------------------*/
void enable_sigio()
{
  sigemptyset(&set);
  sigaddset(&set,SIGIO);
  sigprocmask(SIG_UNBLOCK, &set, NULL);
  sigflag = 1;
}

void enable_sigalrm()
{
  alarmdisabled = 0;
  sigemptyset(&set);
  sigaddset(&set,SIGALRM);
  sigprocmask(SIG_UNBLOCK, &set, NULL);
}

/*-----------------------------------------------------------*/
void jia_error(char *str, ...)
{
  va_list ap;
  va_start(ap, str);
  vsprintf(errstr, str, ap);
  va_end(ap);
  assert(0, errstr);
}

#else  /* NULL_LIB */
#include <sys/time.h>

unsigned long start_sec = 0; 
unsigned long start_time_sec  = 0; 
unsigned long start_time_usec  = 0; 
unsigned long start_msec = 0;  
float jia_clock()
{
  struct timeval val;
  gettimeofday(&val, NULL);
  if (!start_time_sec) {
    start_time_sec = val.tv_sec;
    start_time_usec = val.tv_usec;
  }
  return (float)(((val.tv_sec * 1000000.0 + val.tv_usec) -
         (start_time_sec * 1000000.0 - start_time_usec)) / 1000000.0);
}

void jia_error(char *errstr)
{
  printf("JUMP error --- %s\n", errstr);
  exit(-1);
}
#endif /* NULL_LIB */

/*-----------------------------------------------------------*/
unsigned int get_usecs()
{
#ifdef AIX41
  register unsigned int seconds asm("5");
  register unsigned int nanosecs asm("6");
  register unsigned int seconds2 asm("7");

  /* Need to read the first value twice to make sure it doesn't role
   *   over while we are reading it.
   */
  retry:
  asm("mfspr   5,4");     /* read high into register 5 - seconds */
  asm("mfspr   6,5");     /* read low into register 6 - nanosecs */
  asm("mfspr   7,4");     /* read high into register 7 - seconds2 */

  if (seconds != seconds2) {
    goto retry;
  }

  /* convert to correct form. */

  return seconds * 1000000 + nanosecs / 1000;
#else  /* AIX41 */
  static struct timeval base;
  struct timeval time;

  gettimeofday(&time, NULL);
  if (!base.tv_usec) {
    base = time;
  }
  return ((time.tv_sec - base.tv_sec) * 1000000 +
          (time.tv_usec - base.tv_usec));
#endif /* AIX41 */
}
