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
 * $Log: syn.c,v $
 * Revision 1.2  1998/03/06 05:19:51  dsm
 * add state statistics.
 *
 * Revision 1.2  1998/03/05 08:02:08  rasit
 * New stats collection
 *
 * Revision 1.1  1998/03/05 07:47:31  rasit
 * *** empty log message ***
 *
 * Revision 1.1  1998/03/05 06:28:44  dsm
 * pause instead of while(1) when waiting.
 *
 * Revision 1.0  1998/02/19 06:35:20  rasit
 * Initial revision
 *
 **********************************************************************/

#ifndef NULL_LIB
#include "global.h"
#include "init.h"
#include "mem.h"
#include "syn.h"
#include "comm.h"
#include "assert.h"

extern jia_msg_t *newmsg();
extern void freemsg(jia_msg_t*);
extern void printmsg(jia_msg_t *msg,int right);
extern void asendmsg(jia_msg_t *req);

extern void insert(int i);
extern void insert2(int i);
extern void insert3(int i);
extern void dislink(int i);
extern void dislink2(int i);
extern void dislink3(int i);
extern short int head, head2, head3, tail, tail2, tail3;
extern short int pendprev[Maxmempages], pendnext[Maxmempages];
extern short int pendprev2[Maxmempages], pendnext2[Maxmempages];
extern short int pendprev3[Cachepages], pendnext3[Cachepages];
extern void disable_sigio_sigalrm();
extern void enable_sigio();
extern void appendmsg(jia_msg_t *msg, const void *str, int len);
extern void senddiffs();

int magicflag, magicno, magicarray[Maxmempages];
int magicwtnt[Maxmempages], magicoffset[Maxmempages];
wtnt_t *endptr;
void readwtnt(wtnt_t *ptr, int arg1);
int lastrunner, nowrunner;
int sigflag;
int bank, leftout;

extern void savediff(int cachei, int dflag);
extern void savehomechange();
void savepage(int cachei);

extern wtnt_t *newwtnt();
extern void freewtntspace(wtnt_t *ptr);
extern void freetwin(address_t twin);

void jia_wait();
void waitserver(jia_msg_t *);
void waitgrantserver(jia_msg_t *);
void initsyn();
void clearlocks();
void savewtnt(wtnt_t *ptr, int pagei, int frompid);
void savecontext(int);
void invalidate(jia_msg_t *req);
void acquire(int lock);
void pushstack(int lock);
void jia_lock(int lock);
wtnt_t *copylockwtnts(jia_msg_t *msg, wtnt_t *ptr);
wtnt_t *copystackwtnts(jia_msg_t *msg, wtnt_t *ptr);
void sendwtnts(int operation);
void popstack(int lock);
void jia_unlock(int lock);
void jia_barrier();
void grantlock(long lock, int toproc, int operation);
void acqserver(jia_msg_t *req);
void relserver(jia_msg_t *req);
void grantbarr();
void barrserver(jia_msg_t *req);
void recordwtnts(jia_msg_t *req);
void wtntserver(jia_msg_t *req);
void invalidate(jia_msg_t *req);
void invserver(jia_msg_t *req);
void barrgrantserver(jia_msg_t *req);
void acqgrantserver(jia_msg_t *req);
void acqfwdserver(jia_msg_t *req);
int alarming, alarmdisabled;

extern int numberofpages;
extern int value;
extern int jia_pid;
extern host_t hosts[Maxhosts];
extern int hostc;
extern int oaccess[Maxlocks][LENGTH];
extern jiapage_t page[Maxmempages + 1];
extern jiacache_t cache[Cachepages + 1];

extern volatile int diffwait;

#ifdef DOSTAT
extern jiastat_t jiastat;
#endif

jialock_t locks[Maxlocks + Maxcondvs + 1];
jiastack_t lockstack[Maxstacksize];
int stackptr;
volatile int acqwait;
volatile int barrwait, barrinterval, noclearlocks;
volatile int waitcounter;
volatile int waitwait;
volatile int cvwait;

unsigned short int locksess[Maxlocks];

int *locknext, *locklast, *lockstatus;
int globelock;

#ifdef JT
unsigned int t[70], b[70], s[70], c[70];
#endif

void initsyn()
{
	int i, j;

	sigflag = 1; alarming = 0; alarmdisabled = 0; 
	for (i = 0; i <= Maxlocks + Maxcondvs; i++) {
		locks[i].acqc = 0;
		locks[i].condv = 0;
		for (j = 0; j < Maxhosts; j++)
			locks[i].acqs[j] = -1;
		locks[i].wtntp = newwtnt();
	}

	for (i = 0; i < Maxstacksize; i++) {
		lockstack[i].lockid = 0;
		lockstack[i].wtntp = newwtnt();
	}

	stackptr = 0;
	top.lockid = hidelock;
	waitcounter = 0;
	barrinterval = 0;
	noclearlocks = 0;

	locknext = malloc(Maxlocks * 4);
	lockstatus = malloc(Maxlocks * 4);
	locklast = malloc(Maxlocks * 4);

	for (i = 0; i < Maxlocks; i++) {
		locknext[i] = -1;
		if (i % hostc == jia_pid)
		{
			locklast[i] = jia_pid;
			lockstatus[i] = 0;
		}
		else
		{
			locklast[i] = -1;
			lockstatus[i] = -1;
		}
	}
	globelock = -1;

	magicno = 0;
	magicflag = 0;
	for (i = 0; i < Maxmempages; i++)
		magicarray[i] = 0;
	lastrunner = nowrunner = 0;

	bank = leftout = -1;

#ifdef JT
	for (i = 0; i < 70; i++) {
		b[i] = c[i] = t[i] = 0;
		s[i] = 99999999;
	}
#endif
}


void clearlocks()
{
	int i;

	for (i = jia_pid; i < (Maxlocks + Maxcondvs); i += hostc) {
		freewtntspace(locks[i].wtntp);
	}
}

void jia_lock(int lock)
{
	int i;
	sigset_t oldset, set;

#ifdef DOSTAT
	register unsigned int begin;
#endif

#ifdef JT
	register unsigned int x1, x2, x3, x4, x5, x6, x7;
#endif

	if (hostc == 1)
		return;

#ifdef JT
	c[1]++; 
	x1 = get_usecs();
#endif

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, &oldset);

#ifdef DOSTAT
	begin = get_usecs();
	jiastat.lockcnt++;
	jiastat.kernelflag = 1;
#endif

	RASSERT(((lock >= 0) && (lock < Maxlocks)), "lock %d should range from 0 to %d", lock, Maxlocks - 1);

	for (i = 0; i <= stackptr; i++)
		RASSERT((lockstack[i].lockid != lock), "Embedding of the same lock is not allowed!");

	bank = lock;

#ifdef JT
	c[2]++; x2 = get_usecs();
#endif

	savecontext(ACQ);

#ifdef JT
	x3 = get_usecs();
#endif

	acqwait = 1;

#ifdef JT
	c[3]++; x4 = get_usecs();
#endif

	acquire(lock);

#ifdef JT
	x5 = get_usecs();
#endif

	pushstack(lock);

#ifdef JT
	c[4]++; 
	x6 = get_usecs();
#endif

#ifdef DOSTAT
	jiastat.locktime += get_usecs() - begin;
	jiastat.kernelflag = 0;
#endif

#ifdef JT
	x7 = get_usecs();
	t[1] += (x7 - x1); t[2] += (x3 - x2); 
	t[3] += (x5 - x4); t[4] += (x7 - x6);
	if (x7 - x1 > b[1]) b[1] = x7 - x1;
	if (x3 - x2 > b[2]) b[2] = x3 - x2;
	if (x5 - x4 > b[3]) b[3] = x5 - x4;
	if (x7 - x6 > b[4]) b[4] = x7 - x6;
	if (x7 - x1 < s[1]) s[1] = x7 - x1;
	if (x3 - x2 < s[2]) s[2] = x3 - x2;
	if (x5 - x4 < s[3]) s[3] = x5 - x4;
	if (x7 - x6 < s[4]) s[4] = x7 - x6;
#endif

	sigprocmask(SIG_SETMASK, &oldset, NULL);
}

void jia_unlock(int lock)
{
	int locallock, oldlock, wtnti;
	wtnt_t *wnptr;
	sigset_t oldset, set;

#ifdef DOSTAT
	register unsigned int begin;
#endif

#ifdef JT
	register unsigned int x1, x2, x3, x4, x5;
#endif

	if (hostc == 1)
		return;

#ifdef JT
	c[5]++;
	x1 = get_usecs();
#endif

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, &oldset);

#ifdef DOSTAT
	begin = get_usecs();
	jiastat.kernelflag = 1;
#endif

	RASSERT(((lock >= 0) && (lock < Maxlocks)), "lock %d should range from 0 to %d", lock, Maxlocks - 1);

	RASSERT((lock == top.lockid), "lock and unlock should be used in pair!");

	disable_sigio_sigalrm();
	if (leftout != -1) {
		if (stackptr >= 0) {
			locallock = top.lockid;  
			oldlock = lockstack[stackptr + 1].lockid;
			wnptr = locks[oldlock].wtntp;
			readwtnt(locks[locallock].wtntp, 2);
			while(wnptr != WNULL) {
				for (wtnti = 0; wtnti < wnptr->wtntc; wtnti++) {
					savewtnt(locks[locallock].wtntp,
							wnptr->wtnts[wtnti], Maxhosts);
				}
				wnptr = wnptr->more; 
			}
			magicflag = 0;
		}
		leftout = -1;
	}
	enable_sigio();

#ifdef JT
	c[6]++;
	x2 = get_usecs();
#endif

	globelock = lock;
	savecontext(REL);

#ifdef JT
	x3 = get_usecs();
#endif

#ifdef JT
	c[7]++;
	x4 = get_usecs();
#endif

	popstack(lock);

#ifdef JT
	x5 = get_usecs();
#endif

	bank = lock;

#ifdef DOSTAT
	jiastat.unlocktime += get_usecs() - begin;
	jiastat.kernelflag = 0;
#endif

#ifdef JT
	t[5] += (x5-x1); t[6] += (x3-x2); t[7] += (x5-x4);
	if (x5-x1 > b[5]) b[5] = x5-x1;
	if (x3-x2 > b[6]) b[6] = x3-x2;
	if (x5-x4 > b[7]) b[7] = x5-x4;
	if (x5-x1 < s[5]) s[5] = x5-x1;
	if (x3-x2 < s[6]) s[6] = x3-x2;
	if (x5-x4 < s[7]) s[7] = x5-x4;
#endif

	sigprocmask(SIG_SETMASK, &oldset, NULL);
}


void jia_barrier()
{
	int oldlock, locallock, wtnti, i, tempstackptr, tempstack[10];
	wtnt_t *wnptr;
	sigset_t oldset, set;

#ifdef DOSTAT
	register unsigned int begin;
#endif

#ifdef JT
	register unsigned int x1, x2, x3, x4, x5, x6, x7, x8, x9;
#endif

	if (hostc == 1) return;

#ifdef JT
	c[8]++;
	x1 = get_usecs();
#endif

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, &oldset);

#ifdef DOSTAT
	begin = get_usecs();
	jiastat.barrcnt++;
	jiastat.kernelflag = 1;
#endif

	tempstackptr = stackptr;
	for (i = tempstackptr; i >= 1; i--) {
		tempstack[i] = lockstack[i].lockid;
		printf("Auto unlocking %d\n", tempstack[i]);
		jia_unlock(tempstack[i]);
	}

	disable_sigio_sigalrm();
	if (leftout != -1) {
		if (stackptr >= 0) {
			locallock = top.lockid;  
			oldlock = lockstack[stackptr + 1].lockid;
			wnptr = locks[oldlock].wtntp;
			readwtnt(locks[locallock].wtntp, 2);
			while(wnptr != WNULL) {
				for (wtnti = 0; wtnti < wnptr->wtntc; wtnti++) {
					savewtnt(locks[locallock].wtntp,
							wnptr->wtnts[wtnti], Maxhosts);
				}
				wnptr = wnptr->more; 
			}
			magicflag = 0;
		}
		leftout = -1;
	}
	enable_sigio();

#ifdef JT
	c[9]++; 
	x2 = get_usecs();
#endif

	savecontext(BARR);

#ifdef JT
	x3 = get_usecs();
#endif

	barrwait = 1;

#ifdef JT
	c[10]++; 
	x4 = get_usecs();
#endif

	sendwtnts(BARR);

#ifdef JT
	x5 = get_usecs();
	c[11]++;
	x6 = get_usecs();
#endif

	freewtntspace(top.wtntp);

#ifdef JT
	x7 = get_usecs();
	c[12]++;
	x8 = get_usecs();
#endif

	enable_sigio();

	while(barrwait) {
		if (sigflag == 0) {
			enable_sigio();
		}
	}

	for (i = 1; i <= tempstackptr; i++) {
		printf("Auto re-locking %d\n", tempstack[i]);
		jia_lock(tempstack[i]);
	}

#ifdef JT
	x9 = get_usecs();
#endif

#ifdef JT
	t[8] += (x9 - x1); t[9] += (x3 - x2); t[10] += (x5 - x4);
	t[11] += (x7 - x6); t[12] += (x9 - x8);
	if (x9 - x1 > b[8]) b[8] = x9 - x1;
	if (x3 - x2 > b[9]) b[9] = x3 - x2;
	if (x5 - x4 > b[10]) b[10] = x5 - x4;
	if (x7 - x6 > b[11]) b[11] = x7 - x6;
	if (x9 - x8 > b[12]) b[12] = x9 - x8;
	if (x5 - x1 < s[8]) s[8] = x9 - x1;
	if (x3 - x2 < s[9]) s[9] = x3 - x2;
	if (x5 - x4 < s[10]) s[10] = x5 - x4;
	if (x7 - x6 < s[11]) s[11] = x7 - x6;
	if (x9 - x8 < s[12]) s[12] = x9 - x8;
#endif

#ifdef DOSTAT
	jiastat.barrtime += get_usecs() - begin;
	jiastat.kernelflag = 0;
#endif

	sigprocmask(SIG_SETMASK, &oldset, NULL);
}

void jia_wait()
{
	jia_msg_t *req;

	if (hostc == 1)
		return;

	req = newmsg();
	req->frompid = jia_pid;
	req->topid = 0;
	req->op = WAIT;
	req->size = 0;

	waitwait = 1;
	asendmsg(req);
	freemsg(req);
	while(waitwait) ;
}

void savecontext(int synop)
{
	register int cachei, pagei;
	int hosti, tmp, tmp3[Cachepages], tmp3a[Cachepages];
	int locallock, i, count, counta;

#ifdef JT
	register unsigned int x1, x2, x3, x4, x5, x6, x7, x8;
	register unsigned int y1, y2, y3, y4, y5, y6, y7, y8;
#endif

	pagei = head2;
	while(pagei != -1) {
		if (page[pagei].state == RW)
			page[pagei].state = RO;
		pagei = pendnext2[pagei];
	}

	cachei = head3;
	counta = 0;
	count = 0;
	disable_sigio_sigalrm();
	while(cachei != -1) {
		if (cache[cachei].state != RO)
			tmp3a[counta++] = cachei;
		else
			tmp3[count++] = cachei;
		tmp = cachei;
		cachei = pendnext3[cachei];
		dislink3(tmp);
	}
	locallock = top.lockid;
	readwtnt(locks[locallock].wtntp, 3);
	for (i = 0; i < counta; i++) {
		cachei = tmp3a[i];

#ifdef JT
		if (synop == ACQ) c[16]++;
		else if (synop == REL) c[17]++;
		else if (synop == BARR) c[18]++;
		x3 = get_usecs();
#endif

		savepage(cachei);

#ifdef JT
		x4 = get_usecs();
		if (synop == ACQ) {
			t[16] += (x4 - x3);
			if (x4 - x3 > b[16]) b[16] = x4 - x3;
			if (x4 - x3 < s[16]) s[16] = x4 - x3;
		}
		else if (synop == REL) {
			t[17] += (x4 - x3);
			if (x4 - x3 > b[17]) b[17] = x4 - x3;
			if (x4 - x3 < s[17]) s[17] = x4 - x3;
		}
		else if (synop == BARR) {
			t[18] += (x4 - x3);
			if (x4 - x3 > b[18]) b[18] = x4 - x3;
			if (x4 - x3 < s[18]) s[18] = x4 - x3;
		}
#endif

		hosti = homehost(cache[cachei].addr);
		if (hosti != jia_pid) {

#ifdef JT
			if (synop == ACQ) c[19]++;
			else if (synop == REL) c[20]++;
			else if (synop == BARR) c[21]++;
			x5 = get_usecs();
#endif

			freetwin(cache[cachei].twin);

#ifdef JT
			x6 = get_usecs();
			if (synop == ACQ) {
				t[19] += (x6 - x5);
				if (x6 - x5 > b[19]) b[19] = x6 - x5;
				if (x6 - x5 < s[19]) s[19] = x6 - x5;
			}
			else if (synop == REL) {
				t[20] += (x6 - x5);
				if (x6 - x5 > b[20]) b[20] = x6 - x5;
				if (x6 - x5 < s[20]) s[20] = x6 - x5;
			}
			else if (synop == BARR) {
				t[21] += (x6 - x5);
				if (x6 - x5 > b[21]) b[21] = x6 - x5;
				if (x6 - x5 < s[21]) s[21] = x6 - x5;
			}
#endif

		}
		cache[cachei].state = R1;
		memprotect((caddr_t)cache[cachei].addr, Pagesize, PROT_READ);
	}
	magicflag = 0;
	enable_sigio();

	for (i = 0; i < count; i++) {
		cachei = tmp3[i];
		cache[cachei].wtnt = 0;

#ifdef JT
		if (synop == ACQ) c[13]++;
		else if (synop == REL) c[14]++;
		else if (synop == BARR) c[15]++;
		x1 = get_usecs();
#endif
		savediff(cachei, 0);
#ifdef JT
		x2 = get_usecs();
		if (synop == ACQ) { 
			t[13] += (x2 - x1);
			if (x2 - x1 > b[13]) b[13] = x2 - x1;
			if (x2 - x1 < s[13]) s[13] = x2 - x1;
		}
		else if (synop == REL) {
			t[14] += (x2 - x1);
			if (x2 - x1 > b[14]) b[14] = x2 - x1;
			if (x2 - x1 < s[14]) s[14] = x2 - x1;
		}
		else if (synop == BARR) {
			t[15] += (x2 - x1);
			if (x2 - x1 > b[15]) b[15] = x2 - x1;
			if (x2 - x1 < s[15]) s[15] = x2 - x1;
		}
#endif

	}
	for (i = 0; i < counta; i++) {
		cachei = tmp3a[i];
		cache[cachei].wtnt = 0;

#ifdef JT
		if (synop == ACQ) c[13]++;
		else if (synop == REL) c[14]++;
		else if (synop == BARR) c[15]++;
		x1 = get_usecs();
#endif
		savediff(cachei, 1);
#ifdef JT
		x2 = get_usecs();
		if (synop == ACQ) { 
			t[13] += (x2 - x1);
			if (x2 - x1 > b[13]) b[13] = x2 - x1;
			if (x2 - x1 < s[13]) s[13] = x2 - x1;
		}
		else if (synop == REL) {
			t[14] += (x2 - x1);
			if (x2 - x1 > b[14]) b[14] = x2 - x1;
			if (x2 - x1 < s[14]) s[14] = x2 - x1;
		}
		else if (synop == BARR) {
			t[15] += (x2 - x1);
			if (x2 - x1 > b[15]) b[15] = x2 - x1;
			if (x2 - x1 < s[15]) s[15] = x2 - x1;
		}
#endif
	}

#ifdef JT
	if (synop == ACQ)
		c[22]++;
	else if (synop == REL)
		c[23]++;
	else if (synop == BARR)
		c[24]++;
	x7 = get_usecs();
#endif

	savehomechange();

#ifdef JT
	x8 = get_usecs();
	if (synop == ACQ) {
		t[22] += (x8 - x7);
		if (x8 - x7 > b[22]) b[22] = x8 - x7;
		if (x8 - x7 < s[22]) s[22] = x8 - x7;
	} else if (synop == REL) {
		t[23] += (x8 - x7);
		if (x8 - x7 > b[23]) b[23] = x8 - x7;
		if (x8 - x7 < s[23]) s[23] = x8 - x7;
	} else if (synop == BARR) {
		t[24] += (x8 - x7);
		if (x8 - x7 > b[24]) b[24] = x8 - x7;
		if (x8 - x7 < s[24]) s[24] = x8 - x7;
	}
#endif

#ifdef JT
	if (synop == ACQ) c[25]++;
	else if (synop == REL) c[26]++;
	else if (synop == BARR) c[27]++;
	y1 = get_usecs();
#endif

	senddiffs();

#ifdef JT
	y2 = get_usecs();
	if (synop == ACQ) {
		t[25] += (y2 - y1);
		if (y2 - y1 > b[25]) b[25] = y2 - y1;
		if (y2 - y1 < s[25]) s[25] = y2 - y1;
	}
	else if (synop == REL) {
		t[26] += (y2 - y1);
		if (y2 - y1 > b[26]) b[26] = y2 - y1;
		if (y2 - y1 < s[26]) s[26] = y2 - y1;
	}
	else if (synop == BARR) {
		t[27] += (y2 - y1);
		if (y2 - y1 > b[27]) b[27] = y2 - y1;
		if (y2 - y1 < s[27]) s[27] = y2 - y1;
	}
#endif

	pagei = head2;
	disable_sigio_sigalrm();
	locallock = top.lockid;
	readwtnt(locks[locallock].wtntp, 1);
	while(pagei != -1) {
		if (page[pagei].rdnt == 1) {

#ifdef JT
			if (synop == ACQ) c[28]++;
			else if (synop == REL) c[29]++;
			else if (synop == BARR) c[30]++;
			y3 = get_usecs();
#endif

			savewtnt(locks[locallock].wtntp, pagei, Maxhosts);
			if (synop == BARR) page[pagei].rdnt = 0;

#ifdef JT
			y4 = get_usecs();
			if (synop == ACQ) {
				t[28] += (y4 - y3);
				if (y4 - y3 > b[28]) b[28] = y4 - y3;
				if (y4 - y3 < s[28]) s[28] = y4 - y3;
			}
			else if (synop == REL) {
				t[29] += (y4 - y3);
				if (y4 - y3 > b[29]) b[29] = y4 - y3;
				if (y4 - y3 < s[29]) s[29] = y4 - y3;
			}
			else if (synop == BARR) {
				t[30] += (y4 - y3);
				if (y4 - y3 > b[30]) b[30] = y4 - y3;
				if (y4 - y3 < s[30]) s[30] = y4 - y3;
			}
#endif

		}

#ifdef JT
		if (synop == ACQ) c[31]++;
		else if (synop == REL) c[32]++;
		else if (synop == BARR) c[33]++;
		y5 = get_usecs();
#endif

		memprotect((caddr_t)page[pagei].addr, Pagesize, PROT_READ);

#ifdef JT
		y6 = get_usecs();
		if (synop == ACQ) {
			t[31] += (y6 - y5);
			if (y6 - y5 > b[31]) b[31] = y6 - y5;
			if (y6 - y5 < s[31]) s[31] = y6 - y5;
		}
		else if (synop == REL) {
			t[32] += (y6 - y5);
			if (y6 - y5 > b[32]) b[32] = y6 - y5;
			if (y6 - y5 < s[32]) s[32] = y6 - y5;
		}
		else if (synop == BARR) {
			t[33] += (y6 - y5);
			if (y6 - y5 > b[33]) b[33] = y6 - y5;
			if (y6 - y5 < s[33]) s[33] = y6 - y5;
		}
#endif

		tmp = pendnext2[pagei];
		page[pagei].wtnt = 0;
		dislink2(pagei);
		if (pagei == tmp) printf("Error: Infinite loop?\n");
		pagei = tmp;
	}

	magicflag = 0;
	enable_sigio();

	disable_sigio_sigalrm();
	pagei = head;
	while (pagei != -1) {
		tmp = pendnext[pagei];
		if (page[pagei].homepid == jia_pid) {
			page[pagei].pend[jia_pid] = 0;
			dislink(pagei);
		}
		if (pagei == tmp) printf("Infinite loop here!\n");
		pagei = tmp;
	}
	enable_sigio();

#ifdef JT
	if (synop == ACQ) c[34]++;
	else if (synop == REL) c[35]++;
	else if (synop == BARR) c[36]++;
	y7 = get_usecs();
#endif

	while(diffwait);

#ifdef JT
	y8 = get_usecs();
	if (synop == ACQ) {
		t[34] += (y8 - y7);
		if (y8 - y7 > b[34]) b[34] = y8 - y7;
		if (y8 - y7 < s[34]) s[34] = y8 - y7;
	}
	else if (synop == REL) {
		t[35] += (y8 - y7);
		if (y8 - y7 > b[35]) b[35] = y8 - y7;
		if (y8 - y7 < s[35]) s[35] = y8 - y7;
	}
	else if (synop == BARR) {
		t[36] += (y8 - y7);
		if (y8 - y7 > b[36]) b[36] = y8 - y7;
		if (y8 - y7 < s[36]) s[36] = y8 - y7;
	}
#endif

}

void pushstack(int lock)
{
	int oldlock, locallock, wtnti;
	wtnt_t *wnptr;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	disable_sigio_sigalrm();
	if (leftout != -1 && leftout != lock) {
		if (stackptr >= 0) {
			locallock = top.lockid;  
			oldlock = lockstack[stackptr + 1].lockid;
			wnptr = locks[oldlock].wtntp;
			readwtnt(locks[locallock].wtntp, 2);
			while(wnptr != WNULL) {
				for (wtnti = 0; wtnti < wnptr->wtntc; wtnti++) {
					savewtnt(locks[locallock].wtntp,
							wnptr->wtnts[wtnti], Maxhosts);
				}
				wnptr=wnptr->more; 
			}
			magicflag = 0;
		}
		leftout = -1;
	}
	enable_sigio();

	stackptr++;
	RASSERT((stackptr < Maxstacksize), "Too many continuous ACQUIRES!");

	top.lockid = lock;

#ifdef JT
	x2 = get_usecs();
	t[58] += (x2 - x1);
	if (x2 - x1 > b[58]) b[58] = x2 - x1;
	if (x2 - x1 < s[58]) s[58] = x2 - x1;
	c[58]++;
#endif

}

void popstack(int lock)
{
	int wtnti, locallock, oldlock;
	wtnt_t *wnptr;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	stackptr--;
	RASSERT((stackptr >= -1),"More unlocks than locks!");

	leftout = lockstack[stackptr + 1].lockid;
	disable_sigio_sigalrm();
	if (locknext[lock] != -1) {
		if (leftout != -1) {
			if (stackptr >= 0) {
				locallock = top.lockid;  
				oldlock = lockstack[stackptr + 1].lockid;
				wnptr = locks[oldlock].wtntp;
				readwtnt(locks[locallock].wtntp, 2);
				while(wnptr != WNULL) {
					for (wtnti = 0; wtnti < wnptr->wtntc; wtnti++) {
						savewtnt(locks[locallock].wtntp,
								wnptr->wtnts[wtnti], Maxhosts);
					}
					wnptr=wnptr->more; 
				}
				magicflag = 0;
			}
			leftout = -1;
		}
		lockstatus[lock] = -1;
		enable_sigio();
		grantlock(lock, locknext[lock], ACQGRANT);
		locknext[lock] = -1;
	}
	else {
		lockstatus[lock] = 0; locknext[lock] = -1;
		enable_sigio();
	}

#ifdef JT
	x2 = get_usecs();
	t[59] += (x2 - x1);
	if (x2 - x1 > b[59]) b[59] = x2 - x1;
	if (x2 - x1 < s[59]) s[59] = x2 - x1;
	c[59]++;
#endif

}


void acquire(int lock)
{
	jia_msg_t *req;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	disable_sigio_sigalrm();
	if (lockstatus[lock] == 1)
		printf("Possible error -- own lock not released\n");
	else
		if (lockstatus[lock] == 0) {   /* I have the lock */
			if (locknext[lock] != -1) 
				printf("locknext %d error? %d\n", lock, locknext[lock]);
			lockstatus[lock] = 1;
			acqwait = 0;
			enable_sigio();
		}
		else {
			req = newmsg();
			if (lock % hostc == jia_pid) {   /* I am the manager */
				req->op = ACQFWD;
				req->frompid = jia_pid;
				req->topid = locklast[lock];
				locklast[lock] = req->frompid;
				req->size = 0;
				appendmsg(req, ltos(lock), Intbytes);
				appendmsg(req, ltos(jia_pid), Intbytes);
				enable_sigio();
				asendmsg(req);
				freemsg(req);
				while(acqwait);
			}
			else {
				req->op = ACQ;
				req->frompid = jia_pid;
				req->topid = lock % hostc;
				req->size = 0;
				appendmsg(req, ltos(lock), Intbytes);
				appendmsg(req, ltos(jia_pid), Intbytes);
				enable_sigio();
				asendmsg(req);
				freemsg(req);
				while(acqwait);
			}
		}

#ifdef JT
	x2 = get_usecs();
	t[57] += (x2 - x1);
	if (x2 - x1 > b[57]) b[57] = x2 - x1;
	if (x2 - x1 < s[57]) s[57] = x2 - x1;
	c[57]++;
#endif

}

void sendwtnts(int operation)
{
	jia_msg_t *req;
	wtnt_t *wnptr; 
	int i, p;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	req=newmsg();
	req->frompid=jia_pid;
	req->topid=top.lockid%hostc;
	req->size=0;
	req->binterval=barrinterval;
	appendmsg(req,ltos(top.lockid),Intbytes);
	p = 0;

	if (top.lockid != Maxlocks + Maxcondvs) {
		printf("Hey! Lock operations shouldn't go here!\n");
		p = (numberofpages + CHARBITS - 1) / CHARBITS;
		appendmsg(req, ltos(p), Intbytes);
		for (i = 0; i < p; i++)
			appendmsg(req, ltos(oaccess[top.lockid][i]), Intbytes);
		p = 0;
	}
	else
		appendmsg(req, ltos(p), Intbytes);

	wnptr = locks[top.lockid].wtntp;
	wnptr = copystackwtnts(req, wnptr);
	while(wnptr != WNULL) {
		req->op = WTNT; 
		asendmsg(req);
		req->size = Intbytes;
		appendmsg(req, ltos(p), Intbytes);
		wnptr = copystackwtnts(req, wnptr);
	}

	req->op = operation;
	asendmsg(req);
	freemsg(req);
	if (top.lockid % hostc != jia_pid)
		freewtntspace(locks[top.lockid].wtntp);

#ifdef JT
	x2 = get_usecs();
	t[54] += (x2 - x1);
	if (x2 - x1 > b[54]) b[54] = x2 - x1;
	if (x2 - x1 < s[54]) s[54] = x2 - x1;
	c[54]++;
#endif

}

void savepage(int cachei)
{
	int locallock, pagei;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	pagei = (int)(((unsigned long)cache[cachei].addr - Startaddr)
			/ Pagesize);

	locallock = top.lockid;
	savewtnt(locks[locallock].wtntp, pagei, Maxhosts);

#ifdef JT
	x2 = get_usecs();
	t[55] += (x2 - x1);
	if (x2 - x1 > b[55]) b[55] = x2 - x1;
	if (x2 - x1 < s[55]) s[55] = x2 - x1;
	c[55]++;
#endif

}

void readwtnt(wtnt_t *ptr, int arg1)
{
	int wtnti, pagei;
	wtnt_t *wnptr;
	int counter;

	lastrunner = nowrunner;
	nowrunner = arg1;

	counter = 0;
	magicno += 1;
	if (magicno == 10000) magicno = 1;
	if (!magicflag) magicflag = 1; 
	else printf("Disrupted ! Last %d Now %d\n", lastrunner, nowrunner);
	wnptr = ptr;
	endptr = ptr;
	while(wnptr != WNULL) {
		for (wtnti = 0; wtnti < wnptr->wtntc; wtnti++) {
			pagei = wnptr->wtnts[wtnti];
			magicarray[pagei] = magicno;
			magicwtnt[pagei] = counter;
			magicoffset[pagei] = wtnti;
		}
		wnptr = wnptr->more;
		if (counter > 0) endptr = endptr->more;
		counter++;
		if (counter >= 100) printf("Possible error in readwtnt\n");
	}
}

void savewtnt(wtnt_t *ptr, int pagei, int frompid)
{
	int exist, i;
	wtnt_t *wnptr;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	if (magicarray[pagei] == magicno)
		exist = 1;
	else
		exist = 0;

	if (exist == 0) {
		wnptr = endptr;
		if (wnptr->wtntc >= Maxwtnts) {
			wnptr->more = newwtnt(); 
			wnptr = wnptr->more;
			endptr = endptr->more;
		}
		wnptr->wtnts[wnptr->wtntc] = pagei;
		wnptr->from[wnptr->wtntc] = frompid;
		wnptr->wtntc++;
	}
	else {   /* Now, we are already to set all to Maxhosts */
		wnptr = ptr;
		for (i = 0; i < magicwtnt[pagei]; i++)
			wnptr = wnptr->more;
		if (wnptr->wtnts[magicoffset[pagei]] != pagei)
			printf("Error: Addr Conflict in savewtnt\n");
		wnptr->from[magicoffset[pagei]] = Maxhosts;
	} 

	if (globelock == 0) {
		globelock = -1; 
	}

#ifdef JT
	x2 = get_usecs();
	t[56] += (x2 - x1);
	if (x2 - x1 > b[56]) b[56] = x2 - x1;
	if (x2 - x1 < s[56]) s[56] = x2 - x1;
	c[56]++;
#endif

}

wtnt_t *copystackwtnts(jia_msg_t *msg, wtnt_t *ptr)
{
	int full;
	wtnt_t *wnptr;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	full = 0;
	wnptr = ptr;
	while((wnptr != WNULL) && (full == 0)) {
		if ((msg->size + (wnptr->wtntc * Intbytes)) < Maxmsgsize) {
			appendmsg(msg, wnptr->wtnts, (wnptr->wtntc * Intbytes));
			wnptr = wnptr->more;   
		}
		else {
			full = 1;
		}
	}

#ifdef JT
	x2 = get_usecs();
	t[52] += (x2-x1);
	if (x2 - x1 > b[52]) b[52] = x2 - x1;
	if (x2 - x1 < s[52]) s[52] = x2 - x1;
	c[52]++;
#endif

	return(wnptr);
}


wtnt_t *copylockwtnts(jia_msg_t *msg, wtnt_t *ptr)
{
	int wtnti;
	int full;
	wtnt_t *wnptr;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	full = 0;
	wnptr = ptr;
	while((wnptr != WNULL) && (full == 0)) {
		if ((msg->size + (wnptr->wtntc * Intbytes)) < Maxmsgsize) {
			for (wtnti = 0; wtnti<wnptr->wtntc; wtnti++) {
				if (wnptr->from[wtnti] != msg->topid)
					appendmsg(msg, ltos(wnptr->wtnts[wtnti]), Intbytes);
			}
			wnptr = wnptr->more;
		}
		else {
			full = 1;
		}
	}

#ifdef JT
	x2 = get_usecs();
	t[51] += (x2 - x1);
	if (x2 - x1 > b[51]) b[51] = x2 - x1;
	if (x2 - x1 < s[51]) s[51] = x2 - x1;
	c[51]++;
#endif

	return(wnptr);
}

void grantlock(long lock, int toproc, int operation)
{
	jia_msg_t *grant;
	wtnt_t *wnptr; 
	int i, p;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	if (lock < Maxlocks)
		locksess[lock]++;

	grant = newmsg();
	grant->frompid = jia_pid;
	grant->topid = toproc;
	grant->size = 0;

	appendmsg(grant, ltos(lock), Intbytes);
	p = 0;

	if (lock < Maxlocks + Maxcondvs) {
		p = (numberofpages + CHARBITS - 1) / CHARBITS;
		appendmsg(grant, ltos(p), Intbytes);
		for (i = 0; i < p; i++)
			appendmsg(grant, ltos(oaccess[lock][i]), Intbytes);
		p = 0;
	}
	else
		appendmsg(grant, ltos(p), Intbytes);

	wnptr = locks[lock].wtntp;
	wnptr = copylockwtnts(grant, wnptr);
	while(wnptr != WNULL) {
		grant->op = INVLD; 
		asendmsg(grant);
		grant->size = Intbytes;
		appendmsg(grant, ltos(p), Intbytes);
		wnptr = copylockwtnts(grant, wnptr);
	}

	grant->op = operation;
	asendmsg(grant);
	freemsg(grant);

	if (lock < Maxcondvs + Maxlocks)
		freewtntspace(locks[lock].wtntp);

#ifdef JT
	x2 = get_usecs();
	t[53] += (x2 - x1);
	if (x2 - x1 > b[53]) b[53] = x2 - x1;
	if (x2 - x1 < s[53]) s[53] = x2 - x1;
	c[53]++;
#endif

}

void acqfwdserver(jia_msg_t *req)
{
	long lock, proc, oldlock, locallock, wtnti;
	wtnt_t *wnptr;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	lock = (int)stol(req->data);
	proc = (int)stol(req->data + 4);
	if (lockstatus[lock] != 0) {
		locknext[lock] = proc;
	}
	else {
		disable_sigio_sigalrm();
		if (leftout != -1) {
			if (stackptr >= 0) {
				locallock = top.lockid;  
				oldlock = lockstack[stackptr + 1].lockid;
				wnptr = locks[oldlock].wtntp;
				readwtnt(locks[locallock].wtntp, 2);
				while(wnptr != WNULL) {
					for (wtnti = 0; wtnti < wnptr->wtntc; wtnti++) {
						savewtnt(locks[locallock].wtntp,
								wnptr->wtnts[wtnti], Maxhosts);
					}
					wnptr = wnptr->more; 
				}
				magicflag = 0;
			}
			leftout = -1;
		}
		enable_sigio();
		lockstatus[lock] = -1;
		locknext[lock] = -1;
		grantlock(lock, proc, ACQGRANT);
	}

#ifdef JT
	x2 = get_usecs();
	t[42] += (x2 - x1);
	if (x2 - x1 > b[42]) b[42] = x2 - x1;
	if (x2 - x1 < s[42]) s[42] = x2 - x1;
	c[42]++;
#endif

}

void acqserver(jia_msg_t *req)
{
	long lock, from;
	jia_msg_t *rep;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef MHPDEBUG
	RASSERT((req->op == ACQ) && (req->topid == jia_pid), "Incorrect ACQ message!");
#endif

	lock = (int)stol(req->data);
	from = (int)stol(req->data + 4);

#ifdef MHPDEBUG
	RASSERT((lock % hostc == jia_pid),"Incorrect home of lock!");
#endif

	locks[lock].acqs[locks[lock].acqc] = req->frompid;
	locks[lock].acqc++;

	if (locklast[lock] == jia_pid) {
		if (lockstatus[lock] != 0) {
			locknext[lock] = locklast[lock] = req->frompid;
		}
		else {
			lockstatus[lock] = -1;
			locknext[lock] = -1;
			grantlock(lock, req->frompid, ACQGRANT);
		}
	}
	else {   /* It's not the manager's responsibility to grant the lock */
		from = req->frompid;
		rep = newmsg();
		rep->op = ACQFWD;
		rep->frompid = jia_pid;
		rep->topid = locklast[lock];
		rep->size = 0;
		appendmsg(rep, ltos(lock), Intbytes);
		appendmsg(rep, ltos(from), Intbytes);
		asendmsg(rep);
		freemsg(rep);
	}
	locklast[lock] = from;

#ifdef JT
	x2 = get_usecs();
	t[41] += (x2 - x1);
	if (x2 - x1 > b[41]) b[41] = x2 - x1;
	if (x2 - x1 < s[41]) s[41] = x2 - x1;
	c[41]++;
#endif

}

void relserver(jia_msg_t *req)
{
	long lock;
	int acqi;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	printf("I should not be called!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

#ifdef MHPDEBUG
	RASSERT((req->op == REL) && (req->topid == jia_pid), "Incorrect REL Message!"); 
#endif

	lock = (int)stol(req->data);

#ifdef MHPDEBUG
	RASSERT((lock % hostc == jia_pid),"Incorrect home of lock!");
	RASSERT((req->frompid == locks[lock].acqs[0]), "Relserver: This should not have happened!");
#endif

	if (req->binterval > barrinterval) noclearlocks = 1;

	recordwtnts(req); 

	for (acqi = 0; acqi < (locks[lock].acqc - 1); acqi++)
		locks[lock].acqs[acqi] = locks[lock].acqs[acqi + 1];
	locks[lock].acqc--;

	if (locks[lock].acqc > 0)
		grantlock(lock, locks[lock].acqs[0], ACQGRANT);

#ifdef JT
	x2 = get_usecs();
	t[43] += (x2 - x1);
	if (x2 - x1 > b[43]) b[43] = x2 - x1;
	if (x2 - x1 < s[43]) s[43] = x2 - x1;
	c[43]++;
#endif

}

void barrserver(jia_msg_t *req)
{
	long lock;
	int hosti;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef MHPDEBUG
	RASSERT((req->op == BARR) && (req->topid == jia_pid), "Incorrect BARR Message!");
#endif

	lock = (int)stol(req->data);

#ifdef MHPDEBUG
	RASSERT((lock % hostc == jia_pid), "Incorrect home of lock!");
	RASSERT((lock == hidelock), "Barserver: This should not have happened!");
#endif

	recordwtnts(req); 
	locks[lock].acqc++;

	if (locks[lock].acqc == hostc) {
		for (hosti = 0; hosti < hostc; hosti++)
			grantlock(lock, hosti, BARRGRANT);
		locks[lock].acqc = 0;
		freewtntspace(locks[lock].wtntp);
	}

#ifdef JT
	x2 = get_usecs();
	t[44] += (x2 - x1);
	if (x2 - x1 > b[44]) b[44] = x2 - x1;
	if (x2 - x1 < s[44]) s[44] = x2 - x1;
	c[44]++;
#endif

}

void waitserver(jia_msg_t *req)
{
	jia_msg_t *grant;
	int i;

	RASSERT((req->op == WAIT) && (req->topid == jia_pid), "Incorrect WAIT Message!");
	waitcounter++;

	if (waitcounter == hostc) {
		grant = newmsg();
		waitcounter = 0;
		grant->frompid = jia_pid;
		grant->size = 0;
		grant->op = WAITGRANT;
		for (i = 0; i < hostc; i++) {
			grant->topid = i;
			asendmsg(grant);
		}
		freemsg(grant);
	}
}

void recordwtnts(jia_msg_t *req)
{
	int lock;
	int datai;
	int i, count, index;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	lock=(int)stol(req->data);
	index = Intbytes;

	count = (int)stol(req->data + index);
	index += Intbytes;
	if (count > 0) {  

#ifdef MHPDEBUG
		RASSERT((lock < Maxlocks + Maxcondvs), "Error in recordwtnts!\n");
#endif

		for (i = 0; i < count; i++) {
			oaccess[lock][i] = stol(req->data+index);
			index += Intbytes;
		}
	}

	disable_sigio_sigalrm();
	readwtnt(locks[lock].wtntp, 4);
	for (datai = index; datai < req->size; datai += Intbytes)
		savewtnt(locks[lock].wtntp, (int)stol(req->data + datai),
				req->frompid);
	magicflag = 0;
	enable_sigio();

#ifdef JT
	x2 = get_usecs();
	t[49] += (x2 - x1);
	if (x2 - x1 > b[49]) b[49] = x2 - x1;
	if (x2 - x1 < s[49]) s[49] = x2 - x1;
	c[49]++;
#endif

} 

void wtntserver(jia_msg_t *req)
{

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef MHPDEBUG
	RASSERT((req->op == WTNT) && (req->topid == jia_pid), "Incorrect WTNT Message!"); 
#endif


#ifdef MHPDEBUG
	int lock = (int)stol(req->data);
	RASSERT((lock % hostc == jia_pid), "Incorrect home of lock!");
#endif

	recordwtnts(req); 

#ifdef JT
	x2 = get_usecs();
	t[47] += (x2 - x1);
	if (x2 - x1 > b[47]) b[47] = x2 - x1;
	if (x2 - x1 < s[47]) s[47] = x2 - x1;
	c[47]++;
#endif

} 

void invalidate(jia_msg_t *req)
{
	int cachei;
	int lock;
	int datai;
	address_t addr;
	int pagei;
	int i, count, index;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	lock = (int)stol(req->data);
	index = Intbytes;
	count = (int)stol(req->data + index);
	index += Intbytes;
	if (count > 0) {

#ifdef MHPDEBUG
		RASSERT((lock < Maxlocks + Maxcondvs), "Invalidate: Unexpected error!\n");
#endif

		for (i = 0; i < count; i++) {
			oaccess[lock][i] = stol(req->data+index);
			index += Intbytes;
		}
	}

	disable_sigio_sigalrm();
	readwtnt(locks[lock].wtntp, 5);

	for (datai = index; datai < req->size; datai += Intbytes) {
		pagei = (int)stol(req->data + datai);
		addr = (address_t)(Startaddr + pagei * Pagesize);
		if (lock < Maxlocks + Maxcondvs)
			savewtnt(locks[lock].wtntp, pagei, Maxhosts);
		if (page[pagei].homepid != jia_pid) {
			cachei = (int)page[((unsigned long)addr 
					- Startaddr) / Pagesize].cachei;
			if (cachei < Cachepages) {
				if (cache[cachei].state != INV) {
					if (cache[cachei].state == RW) freetwin(cache[cachei].twin);
					if (cache[cachei].wtnt == 1) {
						cache[cachei].wtnt=0;
						dislink3(cachei);
					}

					cache[cachei].state=INV;
					if (page[pagei].oldhome == jia_pid)
						page[pagei].state = INV;
					memprotect((caddr_t)cache[cachei].addr, Pagesize, PROT_NONE);

#ifdef DOSTAT
					jiastat.invcnt++;
#endif
				}
			}
			else
				if (page[pagei].state == RO || page[pagei].state == R1
						|| page[pagei].state == RW) {

#ifdef MHPDEBUG
					RASSERT((page[pagei].cachei == Cachepages), "Invalidate: Unexpected Error\n");
#endif

					if (page[pagei].state == RW)
						RASSERT((0 == 1),  "Page %d still RW, quite unexpected.\n", pagei);

					page[pagei].state = INV;
					memprotect((caddr_t)page[pagei].addr, Pagesize, PROT_NONE);

#ifdef DOSTAT
					jiastat.invcnt++;
#endif
				}
		}
	}       /* for */

	magicflag = 0;
	enable_sigio();

#ifdef JT
	x2 = get_usecs();
	t[50] += (x2 - x1);
	if (x2 - x1 > b[50]) b[50] = x2 - x1;
	if (x2 - x1 < s[50]) s[50] = x2 - x1;
	c[50]++;
#endif

}

void invserver(jia_msg_t *req)
{
#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef MHPDEBUG
	RASSERT((req->op == INVLD) && (req->topid == jia_pid), "Incorrect INVLD Message!"); 
#endif

	invalidate(req);

#ifdef JT
	x2 = get_usecs();
	t[48] += (x2 - x1);
	if (x2 - x1 > b[48]) b[48] = x2 - x1;
	if (x2 - x1 < s[48]) s[48] = x2 - x1;
	c[48]++;
#endif

}

void barrgrantserver(jia_msg_t *req)
{
	int i, j, p;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef MHPDEBUG
	RASSERT((req->op == BARRGRANT) && (req->topid == jia_pid), "Incorrect BARRGRANT Message!"); 
#endif

	if (noclearlocks == 0) clearlocks();

	invalidate(req);

	p = (numberofpages + CHARBITS - 1) / CHARBITS;
	for (i = 0; i < Maxlocks; i++)
		for (j = 0; j < p; j++)
			oaccess[i][j] = 0;

	barrwait = 0;
	barrinterval++;
	noclearlocks = 0;

#ifdef JT
	x2 = get_usecs();
	t[45] += (x2 - x1);
	if (x2 - x1 > b[45]) b[45] = x2 - x1;
	if (x2 - x1 < s[45]) s[45] = x2 - x1;
	c[45]++;
#endif

}

void acqgrantserver(jia_msg_t *req)
{
	long lock;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef MHPDEBUG
	RASSERT((req->op == ACQGRANT) && (req->topid == jia_pid), "Incorrect ACQGRANT Message!"); 
#endif

	lock = (int)stol(req->data);
	invalidate(req);
	lockstatus[lock] = 1;
	acqwait = 0;

#ifdef JT
	x2 = get_usecs();
	t[46] += (x2 - x1);
	if (x2 - x1 > b[46]) b[46] = x2 - x1;
	if (x2 - x1 < s[46]) s[46] = x2 - x1;
	c[46]++;
#endif

}

void waitgrantserver(jia_msg_t *req)
{
	RASSERT((req->op == WAITGRANT) && (req->topid == jia_pid), "Incorrect WAITGRANT Message!");
	waitwait = 0;
}

#endif
