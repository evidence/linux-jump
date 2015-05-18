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
 *         Author: Weiwu Hu, Weisong Shi, Zhimin Tang                  *      
 * =================================================================== *
 *   This software is ported to SP2 by                                 *
 *                                                                     *
 *         M. Rasit Eskicioglu                                         *
 *         University of Alberta                                       * 
 *         Dept. of Computing Science                                  *
 *         Edmonton, Alberta T6G 2H1 CANADA                            *
 * =================================================================== *
 * $Log: comm.c,v $
 * Revision 1.3  1998/03/06 09:33:05  dsm
 * add #ifdef DOSTAT in msgserver .
 *
 * Revision 1.2  1998/03/06 05:25:52  dsm
 * delete three server from msgserver program.
 *
 * Revision 1.1  1998/03/06 05:11:37  dsm
 *
 * Revision 1.2  1998/03/05 07:28:59  rasit
 * More stat collection added
 * Buffersize increased with 128 bytes (precaution for overflow)
 *
 * Revision 1.1  1998/02/20 20:42:21  rasit
 * Added code to re-try recvfrom() on EINTR
 *
 * Revision 1.0  1998/02/20 06:07:58  rasit
 * Initial revision
 *
 **********************************************************************/

#ifndef NULL_LIB
#include "comm.h"
#include "mem.h"

/**
 * BEGINCS begins a critical section by disabling SIGIO and SIGALRM signals.
 */
#define  BEGINCS  { sigset_t newmask, oldmask;		\
	sigemptyset(&newmask);				\
	sigaddset(&newmask, SIGIO);			\
	sigaddset(&newmask, SIGALRM);			\
	sigprocmask(SIG_BLOCK, &newmask, &oldmask);	\
	oldsigiomask = sigismember(&oldmask, SIGIO);	\
	sigflag = 0;					\
}

/**
 * ENDCS ends the critical section by restoring the SIGIO signal.
 */
#define  ENDCS    { if (oldsigiomask == 0) enable_sigio();	\
}

#ifndef JIA_DEBUG 
#define msgprint  0 
#else   /* JIA_DEBUG */
#define msgprint  1
#endif  /* JIA_DEBUG */

extern host_t hosts[Maxhosts];
extern int jia_pid; 
extern int hostc;
extern char errstr[Linesize];
extern int msgbusy[Maxmsgs];
extern jia_msg_t msgarray[Maxmsgs];
extern int msgcnt;
extern int sigflag;

/* servers used by asynchronous */
extern void diffserver(jia_msg_t *);
extern void getpserver(jia_msg_t *);
extern void acqserver(jia_msg_t *);
extern void invserver(jia_msg_t *); 
extern void relserver(jia_msg_t *);
extern void jiaexitserver(jia_msg_t *);
extern void wtntserver(jia_msg_t *);
extern void barrserver(jia_msg_t *); 
extern void barrgrantserver(jia_msg_t *);
extern void acqgrantserver(jia_msg_t *);
extern void waitgrantserver(jia_msg_t *);
extern void waitserver(jia_msg_t *);
extern void diffgrantserver(jia_msg_t *);
extern void getpgrantserver(jia_msg_t *);
extern void emptyprintf();

extern void debugmsg(jia_msg_t *, int);
extern jia_msg_t *newmsg();

#ifdef DOSTAT
extern jiastat_t jiastat;
unsigned int interruptflag = 0;
unsigned int intbegin;
#endif

unsigned long reqports[Maxhosts][Maxhosts], repports[Maxhosts][Maxhosts];
CommManager commreq, commrep;
unsigned long timeout_time;
static struct timeval polltime = { 0, 0 };
jia_msg_t inqueue[Maxqueue], outqueue[Maxqueue];
volatile int inhead, intail, incount;
volatile int outhead, outtail, outcount;
long Startport;

char errmsg[80] = {'\0'};

void initcomm();
int req_fdcreate(int, int);
int rep_fdcreate(int, int);

void sigio_handler();

void sigint_handler();
void asendmsg(jia_msg_t *);
void ssendmsg(jia_msg_t *, jia_msg_t *);
void msgserver();
void outsend();

extern void assert(int, char *);
extern void assert0(int, char *);
extern unsigned long jia_current_time();
extern void disable_sigio();
extern void enable_sigio();

extern sigset_t oldset;
int oldsigiomask;

/*---------------------------------------------------------*/
int req_fdcreate(int i, int flag)
{   
	int fd, res, size;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert0((fd != -1), "req_fdcreate()-->socket()");

	/* set the buffer size for send and receive sockets */
	size = (Maxmsgsize + Msgheadsize + 128) * 16;
	res = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
	assert0((res == 0), "req_fdcreate()-->setsockopt(): SO_RCVBUF");

	size = (Maxmsgsize + Msgheadsize + 128) * 16;
	res = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size));
	assert0((res == 0), "req_fdcreate()-->setsockopt(): SO_SNDBUF");

	/* set the port for sending messages to create a channel */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = (flag) ? htons(0) : htons(reqports[jia_pid][i]);

	res = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	assert0((res == 0), "req_fdcreate()-->bind()");
	return fd;
}

/*------------------------------------------------------------*/
int rep_fdcreate(int i, int flag)  /* acknowledgement sockets */
{
	int fd, res;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert0((fd != -1), "rep_fdcreate()-->socket()");

	/* set the port for sending messages to create a channel */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = (flag) ? htons(0) : htons(repports[jia_pid][i]);

	res=bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	assert0((res == 0), "rep_fdcreate()-->bind()");
	return fd;
}

/*------------------------------------------------------------*/
void initcomm()
{
	int i, j, fd;

	msgcnt = 0;	
	for (i = 0; i < Maxmsgs; i++) {
		msgbusy[i] = 0;
		msgarray[i].index = i;
	}

	inhead = 0;
	intail = 0;
	incount = 0;
	outhead = 0;
	outtail = 0;
	outcount = 0;

	/* set up SIGIO and SIGINT handlers */
	{ 
		struct sigaction act;

		act.sa_handler = (void_func_handler)sigio_handler;
		sigemptyset(&act.sa_mask);
		sigaddset(&act.sa_mask, SIGALRM);
		act.sa_flags = SA_RESTART;   /* Must be here for Linux 2.2 */

		if (sigaction(SIGIO, &act, NULL))
			assert0(0, "initcomm()-->sigaction()");

		act.sa_handler = (void_func_handler)sigint_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_NODEFER;
		if (sigaction(SIGINT, &act, NULL)) {
			assert0(0, "segv sigaction problem");  
		}
	}

	/***********Initialize comm ports********************/

	for (i = 0; i < Maxhosts; i++)
		for (j = 0; j < Maxhosts; j++) {
			reqports[i][j] = Startport + i * Maxhosts + j;
			repports[i][j] = Startport + Maxhosts * Maxhosts + i * Maxhosts + j;
		}

	/***********Initialize commreq ********************/

	commreq.rcv_maxfd = 0;
	commreq.snd_maxfd = 0; 
	FD_ZERO(&(commreq.snd_set));
	FD_ZERO(&(commreq.rcv_set));
	for (i = 0; i < Maxhosts; i++) { 
		fd = req_fdcreate(i, 0);
		commreq.rcv_fds[i] = fd;
		FD_SET(fd, &commreq.rcv_set);
		commreq.rcv_maxfd = MAX(fd+1, commreq.rcv_maxfd);

		if (0 > fcntl(commreq.rcv_fds[i], F_SETOWN, getpid()))
			assert0(0, "initcomm()-->fcntl(..F_SETOWN..)");

		if (0 > fcntl(commreq.rcv_fds[i], F_SETFL, FASYNC))
			assert0(0, "initcomm()-->fcntl(..F_SETFL..)");

		fd = req_fdcreate(i,1);
		commreq.snd_fds[i] = fd;
		FD_SET(fd, &commreq.snd_set);
		commreq.snd_maxfd = MAX(fd+1, commreq.snd_maxfd);
	}
	for (i = 0; i < Maxhosts; i++) {
		commreq.snd_seq[i] = 0;
		commreq.rcv_seq[i] = 0;
	}

	/***********Initialize commrep ********************/

	commrep.rcv_maxfd = 0;
	commrep.snd_maxfd = 0; 
	FD_ZERO(&(commrep.snd_set));
	FD_ZERO(&(commrep.rcv_set));

	for(i = 0; i < Maxhosts; i++) { 
		fd = rep_fdcreate(i,0);
		commrep.rcv_fds[i] = fd;
		FD_SET(fd, &(commrep.rcv_set));
		commrep.rcv_maxfd = MAX(fd+1, commrep.rcv_maxfd);

		fd= rep_fdcreate(i,1);
		commrep.snd_fds[i] = fd;
		FD_SET(fd, &commrep.snd_set);
		commrep.snd_maxfd = MAX(fd+1, commrep.snd_maxfd);
	}
}


void acqfwdserver(jia_msg_t *req);

/*----------------------------------------------------------*/
void msgserver()
{

	switch (inqh.op) {
		case REL:       relserver(&inqh);       break;
		case JIAEXIT:   jiaexitserver(&inqh);   break;
		case BARR:      barrserver(&inqh);      break;
		case WTNT:      wtntserver(&inqh);      break;
		case BARRGRANT: barrgrantserver(&inqh); break;
		case ACQGRANT:  acqgrantserver(&inqh);  break;
		case ACQ :      acqserver(&inqh);       break;
		case INVLD:     invserver(&inqh);       break;
		case WAIT:      waitserver(&inqh);      break;
		case WAITGRANT: waitgrantserver(&inqh); break;
		case GETP:      getpserver(&inqh);      break;
		case GETPGRANT: getpgrantserver(&inqh); break;  
		case DIFF:      diffserver(&inqh);      break;
		case DIFFGRANT: diffgrantserver(&inqh); break;
		case ACQFWD:    acqfwdserver(&inqh);    break;
#ifdef DOSTAT
		case STAT:      statserver(&inqh);      break;
		case STATGRANT: statgrantserver(&inqh); break;
#endif 
		default:        debugmsg(&inqh, 1);
				assert0(0, "msgserver(): Incorrect Message!");
				break;
	}
}

/*----------------------------------------------------------*/
void sigint_handler()    /* Invoked by user pressing Ctrl-C */
{
	assert(0,"Exit by user!!\n");
}

/*----------------------------------------------------------*/
void sigio_handler()
{
	int res;
	int i;
	socklen_t s;
	fd_set readfds;
	struct sockaddr_in from, to; 
	sigset_t set, oldset;
	int servemsg;

#ifdef DOSTAT
	register unsigned int begin;
	if (interruptflag == 0) {
		intbegin = get_usecs();
		begin = get_usecs();
		if (jiastat.kernelflag == 0) { 
			jiastat.usersigiocnt++;
		} else if (jiastat.kernelflag == 1) { 
			jiastat.synsigiocnt++;
		} else if (jiastat.kernelflag == 2) { 
			jiastat.segvsigiocnt++; 
		}
	} else {
		jiastat.commtime += get_usecs() - intbegin;
		intbegin = get_usecs();
	}
	interruptflag++;	
#endif

	/* if alarm not disabled, this mean SIGIO is not interrupting DSM */
	/* need to disable SIGALRM and re-enable it later 		    */
	/* otherwise, prev DSM code has already disable SIGALRM 	    */
	/* so, don't disable or enable -- let interrupted DSM do the rest */

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, &oldset);  /* save prev status to oldset */

	disable_sigio();   /* Must be added for Linux 2.2 */

	servemsg=0;
	readfds = commreq.rcv_set;	
	polltime.tv_sec=0;
	polltime.tv_usec=0;
	res = select(commreq.rcv_maxfd, &readfds, NULL, NULL, &polltime);
	/* if (res == 0) printf("Nothing to be done %d\n", servemsg); */

	while (res > 0) {
		for (i = 0; i < hostc; i++) 
			if (i != jia_pid) 
				if (FD_ISSET(commreq.rcv_fds[i], &readfds)) {
					assert0((incount < Maxqueue), 
							"sigio_handler(): Inqueue exceeded!");

					s = sizeof(from);
					res = recvfrom(commreq.rcv_fds[i], (char *)&(inqt),
							Maxmsgsize + Msgheadsize, 0, (struct sockaddr *)&from,
							&s);
					assert0((res >= 0), "sigio_handler()-->recvfrom()");
					to.sin_family = AF_INET;
					memcpy(&to.sin_addr, hosts[inqt.frompid].addr, 
							hosts[inqt.frompid].addrlen);
					to.sin_port = htons(repports[inqt.frompid][inqt.topid]);
					res = sendto(commrep.snd_fds[i], (char *)&(inqt.seqno),
							sizeof(inqt.seqno), 0, (struct sockaddr *)&to, sizeof(to));
					assert0((res != -1), "sigio_handler()-->sendto() ACK");
					if (inqt.seqno > ((signed) commreq.rcv_seq[i])) {

#ifdef DOSTAT
						if (inqt.frompid != inqt.topid) {
							jiastat.msgrcvcnt++;
							jiastat.msgrcvbytes+=(inqt.size+Msgheadsize);
						}
#endif
						commreq.rcv_seq[i] = inqt.seqno;
						debugmsg(&inqt, 1);
						BEGINCS;
						intail = (intail + 1) % Maxqueue;
						incount++;
						if (incount == 1) servemsg = 1;
						ENDCS;
					}
					else {
						debugmsg(&inqt, 1);
						printf("Receive resend message!\n");
					}
				}
		readfds = commreq.rcv_set;	
		polltime.tv_sec = 0;
		polltime.tv_usec = 0;
		res = select(commreq.rcv_maxfd, &readfds, NULL, NULL, &polltime);
	}

#ifdef DOSTAT
	jiastat.commtime += get_usecs() - intbegin;
#endif 

	enable_sigio();

	while (servemsg == 1) {
		msgserver();
		BEGINCS;
		inqh.op = ERRMSG;
		inhead = (inhead + 1) % Maxqueue;
		incount--;
		servemsg = (incount > 0) ? 1 : 0;
		ENDCS;
	}	

#ifdef DOSTAT
	interruptflag--;
	if (interruptflag == 0) { 
		if (jiastat.kernelflag == 0) { 
			jiastat.usersigiotime += get_usecs() - begin;
		} else if (jiastat.kernelflag == 1) {
			jiastat.synsigiotime += get_usecs() - begin;
		} else if (jiastat.kernelflag == 2) {
			jiastat.segvsigiotime += get_usecs()-begin;
		}
	} 
	else intbegin = get_usecs();
#endif /* DOSTAT */

	sigprocmask(SIG_SETMASK, &oldset, NULL);   /* restore original status */
}

/*----------------------------------------------------------*/
void asendmsg(jia_msg_t *msg)
{
	int outsendmsg;

#ifdef DOSTAT
	register unsigned int begin = get_usecs();
#endif

	BEGINCS;
	assert0((outcount < Maxqueue), "asendmsg(): Outqueue exceeded!");
	msg->syn=0;
	memcpy(&(outqt), msg, Msgheadsize + msg->size);
	commreq.snd_seq[msg->topid]++;
	outqt.seqno = commreq.snd_seq[msg->topid];
	outcount++;
	outtail = (outtail + 1) % Maxqueue;
	outsendmsg = (outcount == 1) ? 1 : 0;
	ENDCS;

	while(outsendmsg == 1) {
		outsend();
		BEGINCS;
		outhead = (outhead + 1) % Maxqueue;
		outcount--;
		outsendmsg = (outcount > 0) ? 1 : 0;
		ENDCS;
	}

#ifdef DOSTAT 
	jiastat.commtime += get_usecs() - begin;
	jiastat.asendtime += get_usecs() - begin;
#endif
}

void outsend()
{
	int res, toproc, fromproc;
	struct sockaddr_in to, from;
	int rep;
	int retries_num;
	unsigned long start, end, mytimeout;
	int msgsize;
	socklen_t s;
	int sendsuccess,arrived;
	fd_set readfds;
	int servemsg;

	debugmsg(&outqh, 0);

	toproc = outqh.topid;
	fromproc = outqh.frompid;

	if (toproc == fromproc) {
		BEGINCS;
		assert0((incount <= Maxqueue), "outsend(): Inqueue exceeded!");
		commreq.rcv_seq[toproc] = outqh.seqno;
		memcpy(&(inqt), &(outqh), Msgheadsize + outqh.size);
		debugmsg(&(inqt), 1);
		incount++;
		intail = (intail + 1) % Maxqueue;
		servemsg = (incount == 1) ? 1 : 0;
		ENDCS;   

		while(servemsg == 1) {
			msgserver();
			BEGINCS;
			inqh.op = ERRMSG;
			inhead = (inhead + 1) % Maxqueue;
			incount--;
			servemsg = (incount > 0) ? 1 : 0;
			ENDCS;
		}
	}
	else {
		msgsize = outqh.size + Msgheadsize;

#ifdef DOSTAT
		jiastat.msgsndcnt++;
		jiastat.msgsndbytes += msgsize;
		if (outqh.op == GETP) jiastat.getpcnt++;
		else if (outqh.op == DIFF) jiastat.diffcnt++;
#endif

		to.sin_family = AF_INET;
		memcpy(&to.sin_addr, hosts[toproc].addr, hosts[toproc].addrlen);
		to.sin_port = htons(reqports[toproc][fromproc]);

		retries_num = 0;
		sendsuccess = 0;

		while ((retries_num < MAX_RETRIES) && (sendsuccess != 1)) {
			BEGINCS;
			res=sendto(commreq.snd_fds[toproc], (char *)&(outqh),msgsize, 0,
					(struct sockaddr *)&to, sizeof(to));
			assert0((res != -1), "outsend()-->sendto()");
			ENDCS;

			arrived = 0;

			/* This is added to avoid Ethernet packet collision */
			/* every machine (16) has a different timeout value */
			if (hostc <= 8)
				mytimeout = TIMEOUT - jia_pid * 100;
			else
				mytimeout = TIMEOUT - jia_pid * 50;
			start= jia_current_time();
			end  = start + mytimeout;

			while ((jia_current_time() < end) && (arrived != 1)) {
				BEGINCS;
				FD_ZERO(&readfds);
				FD_SET(commrep.rcv_fds[toproc], &readfds);
				polltime.tv_sec = 0;
				polltime.tv_usec = 0;
				res = select(commrep.rcv_maxfd, &readfds, NULL, NULL, &polltime);
				if (res == -1) printf("SELECT: res -1, error %d\n", errno);
				if (FD_ISSET(commrep.rcv_fds[toproc], &readfds) != 0)
					arrived = 1;
				ENDCS;
			}

			if (arrived == 1) {

recv_again:
				BEGINCS;
				s= sizeof(from);
				res = recvfrom(commrep.rcv_fds[toproc], (char *)&rep, Intbytes,0,
						(struct sockaddr *)&from, &s);
				ENDCS;
				if ((res < 0) && (errno == EINTR))
				{   printf("Hang up? when recv from %d, res = %d\n", toproc, res);
					goto recv_again;
				}
				if ((res != -1) && (rep == outqh.seqno)) sendsuccess=1;
				else printf("send problem: res = %d, rep = %d\n", res, rep);
			}

			if (sendsuccess == 1) {
			}
			else
				retries_num++;
		}

		if (sendsuccess != 1) {
			printf("Going to can't send error!, sendsuccess = %d\n",
					sendsuccess);
			printf("Outqueue head size %d, mesg size %d\n", outqh.size, msgsize);
			sprintf(errstr, "Can't asend message (%d,%d) to host %d!", outqh.op,
					outqh.seqno, toproc); 
			assert0((sendsuccess == 1), errstr);
		}
	}
} 
/*-------------------------------------------*/
#endif /* NULL_LIB */
