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
 *            Author: Weiwu Hu, Weisong Shi, Zhimin Tang               *       
 * =================================================================== *
 *   This software is ported to SP2 by                                 *
 *                                                                     *
 *         M. Rasit Eskicioglu                                         *
 *         University of Alberta                                       *
 *         Dept. of Computing Science                                  *
 *         Edmonton, Alberta T6G 2H1 CANADA                            *
 * =================================================================== *
 * $Log: exit.c,v $
 * Revision 1.1  1998/03/06 05:13:02  dsm
 * add state statistics.
 *
 * Revision 1.1  1998/03/05 07:35:17  rasit
 * Collect and print more stats
 *
 * Revision 1.0  1998/02/19 18:34:02  rasit
 * Initial revision
 *
 **********************************************************************/

#ifndef NULL_LIB
#include "global.h"
#include "init.h"
#include "comm.h"
#include "mem.h"
#include "syn.h"
#include "assert.h"

extern jia_msg_t *newmsg();
extern CommManager commreq, commrep;     
/* it's a good practice to close the FDs after use :) */

extern unsigned long s2l(unsigned char *str);

#ifdef JT
extern unsigned int t[70], b[70], c[70], s[70];
char line[70][40];
#endif

#ifdef DOSTAT
extern jiastat_t jiastat;
jiastat_t allstats[Maxhosts];
extern int jia_pid;
extern host_t hosts[Maxhosts];
extern int hostc;
extern int alloctot;

extern int wfaultcnt;
extern int rfaultcnt;
extern int pagegrantcnt;
extern int homegrantcnt;
extern int refusecnt;
extern int diffmsgcnt;
extern int diffgrantmsgcnt;
extern int dpages;
extern int epages;
int allnew[Maxhosts][17] = { 0 };
extern unsigned int rr1, rr2, rr3, rr4, rw1, rw2, rw3, rw4;

unsigned int totalmsg, totalbyte, totalloc, totalrr1, totalrr2;
unsigned int totalrr3, totalrr4, totalrw1, totalrw2, totalrw3;
unsigned int totalrw4, totaldmsg, totaldiff, totalediff, totalgrant;
unsigned int totalpf1, totalpf2, totalpf3a, totalpf3b, totalpf3c;
unsigned int totalpf3d;

int statcnt=0;
volatile int waitstat;

void statserver(jia_msg_t *rep)
{
	int i;
	jia_msg_t *grant;
	jiastat_t *stat;
	unsigned int temp;

	RASSERT((rep->op == STAT) && (rep->topid == 0), "Incorrect STAT Message!");

	stat = (jiastat_t*)rep->data;
	allstats[rep->frompid].msgsndbytes  = stat->msgsndbytes;
	allstats[rep->frompid].msgrcvbytes  = stat->msgrcvbytes;
	allstats[rep->frompid].msgsndcnt    = stat->msgsndcnt;
	allstats[rep->frompid].msgrcvcnt    = stat->msgrcvcnt;
	allstats[rep->frompid].segvRcnt     = stat->segvRcnt;
	allstats[rep->frompid].segvLcnt     = stat->segvLcnt;
	allstats[rep->frompid].usersigiocnt = stat->usersigiocnt;
	allstats[rep->frompid].synsigiocnt  = stat->synsigiocnt;
	allstats[rep->frompid].segvsigiocnt = stat->segvsigiocnt;
	allstats[rep->frompid].barrcnt      = stat->barrcnt;
	allstats[rep->frompid].lockcnt      = stat->lockcnt;
	allstats[rep->frompid].getpcnt      = stat->getpcnt;
	allstats[rep->frompid].diffcnt      = stat->diffcnt;
	allstats[rep->frompid].invcnt       = stat->invcnt;
	allstats[rep->frompid].mwdiffcnt    = stat->mwdiffcnt;
	allstats[rep->frompid].swdiffcnt    = stat->swdiffcnt;

	allstats[rep->frompid].barrtime     = stat->barrtime;
	allstats[rep->frompid].segvRtime    = stat->segvRtime;
	allstats[rep->frompid].segvLtime    = stat->segvLtime;
	allstats[rep->frompid].locktime     = stat->locktime;
	allstats[rep->frompid].unlocktime   = stat->unlocktime;
	allstats[rep->frompid].synsigiotime = stat->synsigiotime;
	allstats[rep->frompid].segvsigiotime= stat->segvsigiotime;
	allstats[rep->frompid].usersigiotime= stat->usersigiotime;
	allstats[rep->frompid].endifftime   = stat->endifftime;
	allstats[rep->frompid].dedifftime   = stat->dedifftime;
	allstats[rep->frompid].asendtime    = stat->asendtime;

	/* Follow used by Shi */
	allstats[rep->frompid].busytime = stat->busytime;
	allstats[rep->frompid].datatime = (stat->segvRtime + stat->segvLtime);
	allstats[rep->frompid].syntime = 
		stat->locktime + stat->unlocktime + stat->barrtime;
	if (hosts[rep->frompid].name == "water") {
		allstats[rep->frompid].segvtime = 
			(stat->segvRcnt + stat->segvLcnt) * 300;
		allstats[rep->frompid].othertime = (stat->usersigiocnt) * 300;
	} else {
		allstats[rep->frompid].segvtime = 
			(stat->segvRcnt + stat->segvLcnt) * 600;
		allstats[rep->frompid].othertime = (stat->usersigiocnt) * 600;
	}

	temp = stat->usersigiocnt + stat->synsigiocnt + stat->segvsigiocnt;
	if (hosts[rep->frompid].name == "water") {
		allstats[rep->frompid].commtime = stat->commtime + temp * 300;
	} else {
		allstats[rep->frompid].commtime = stat->commtime + temp * 600;
	}
	allstats[rep->frompid].difftime = 
		allstats[rep->frompid].endifftime + allstats[rep->frompid].dedifftime;

	/* End Shi */

	allnew[rep->frompid][0] = (int)s2l(rep->data + sizeof(jiastat));
	allnew[rep->frompid][1] = (int)s2l(rep->data + sizeof(jiastat) + 4);
	allnew[rep->frompid][2] = (int)s2l(rep->data + sizeof(jiastat) + 8);
	allnew[rep->frompid][3] = (int)s2l(rep->data + sizeof(jiastat) + 12);
	allnew[rep->frompid][4] = (int)s2l(rep->data + sizeof(jiastat) + 16);
	allnew[rep->frompid][5] = (int)s2l(rep->data + sizeof(jiastat) + 20);
	allnew[rep->frompid][6] = (int)s2l(rep->data + sizeof(jiastat) + 24);
	allnew[rep->frompid][7] = (int)s2l(rep->data + sizeof(jiastat) + 28);
	allnew[rep->frompid][8] = (int)s2l(rep->data + sizeof(jiastat) + 32);
	allnew[rep->frompid][9] = (int)s2l(rep->data + sizeof(jiastat) + 36);
	allnew[rep->frompid][10] = (int)s2l(rep->data + sizeof(jiastat) + 40);
	allnew[rep->frompid][11] = (int)s2l(rep->data + sizeof(jiastat) + 44);
	allnew[rep->frompid][12] = (int)s2l(rep->data + sizeof(jiastat) + 48);
	allnew[rep->frompid][13] = (int)s2l(rep->data + sizeof(jiastat) + 52);
	allnew[rep->frompid][14] = (int)s2l(rep->data + sizeof(jiastat) + 56);
	allnew[rep->frompid][15] = (int)s2l(rep->data + sizeof(jiastat) + 60);
	allnew[rep->frompid][16] = (int)s2l(rep->data + sizeof(jiastat) + 64);

	statcnt++;

	if (statcnt == hostc) {
		statcnt = 0;
		clearstat();
		grant = newmsg();
		grant->frompid = jia_pid;
		grant->size = 0;
		grant->op = STATGRANT;
		for(i = 0; i < hostc; i++) {
			grant->topid = i;
			asendmsg(grant);
		}
		freemsg(grant);
	}
}

void statgrantserver(jia_msg_t *req)
{
	RASSERT((req->op == STATGRANT) && (req->topid == jia_pid), "Incorrect STATGRANT Message!");
	waitstat = 0;
}
#endif

void jia_exit()
{
	int i;

#ifdef DOSTAT
	jia_msg_t *reply;
	jiastat_t total;
	jiastat_t *stat_p = &jiastat;
#endif

#ifdef JT
	strcpy(line[1],  "@@ jia_lock    A ");
	strcpy(line[2],  "@@ savecontext A ");
	strcpy(line[3],  "@@ acquire     A ");
	strcpy(line[4],  "@@ pushstack   A ");
	strcpy(line[5],  "@@ jia_unlock  R ");
	strcpy(line[6],  "@@ savecontext R ");
	strcpy(line[7],  "@@ popstack    R ");
	strcpy(line[8],  "@@ jia_barr    B ");
	strcpy(line[9],  "@@ savecontext B ");
	strcpy(line[10], "@@ sendwtnts   B ");
	strcpy(line[11], "@@ freewtnt    B ");
	strcpy(line[12], "@@ while       B ");
	strcpy(line[13], "@@ savediff    A ");
	strcpy(line[14], "@@ savediff    R ");
	strcpy(line[15], "@@ savediff    B ");
	strcpy(line[16], "@@ savepage    A ");
	strcpy(line[17], "@@ savepage    R ");
	strcpy(line[18], "@@ savepage    B ");
	strcpy(line[19], "@@ freetwin    A ");
	strcpy(line[20], "@@ freetwin    R ");
	strcpy(line[21], "@@ freetwin    B ");
	strcpy(line[22], "@@ homechange  A ");
	strcpy(line[23], "@@ homechange  R ");
	strcpy(line[24], "@@ homechange  B ");
	strcpy(line[25], "@@ senddiffs   A ");
	strcpy(line[26], "@@ senddiffs   R ");
	strcpy(line[27], "@@ senddiffs   B ");
	strcpy(line[28], "@@ savewtnts   A ");
	strcpy(line[29], "@@ savewtnts   R ");
	strcpy(line[30], "@@ savewtnts   B ");
	strcpy(line[31], "@@ mprotect    A ");
	strcpy(line[32], "@@ mprotect    R ");
	strcpy(line[33], "@@ mprotect    B ");
	strcpy(line[34], "@@ while       A ");
	strcpy(line[35], "@@ while       R ");
	strcpy(line[36], "@@ while       B ");
	strcpy(line[37], "@@ diffserver    ");
	strcpy(line[38], "@@ diffgrantserv ");
	strcpy(line[39], "@@ getpserver    ");
	strcpy(line[40], "@@ getpgrantserv ");
	strcpy(line[41], "@@ acqserver     ");
	strcpy(line[42], "@@ acqfwdserver  ");
	strcpy(line[43], "@@ relserver     ");
	strcpy(line[44], "@@ barrserver    ");
	strcpy(line[45], "@@ barrgrantserv ");
	strcpy(line[46], "@@ acqgrantserv  ");
	strcpy(line[47], "@@ wtntserver    ");
	strcpy(line[48], "@@ invserver     ");
	strcpy(line[49], "@@ recordwtnts   ");
	strcpy(line[50], "@@ invalidate    ");
	strcpy(line[51], "@@ copylockgrant ");
	strcpy(line[52], "@@ copystkgrant  ");
	strcpy(line[53], "@@ grantlock     ");
	strcpy(line[54], "@@ sendwtnt      ");
	strcpy(line[55], "@@ savepage      ");
	strcpy(line[56], "@@ savewtnt      ");
	strcpy(line[57], "@@ acquire       ");
	strcpy(line[58], "@@ pushstack     ");
	strcpy(line[59], "@@ popstack      ");
	strcpy(line[60], "@@ encodediff    ");
	strcpy(line[61], "@@ savediff      ");
	strcpy(line[62], "@@ savehomechg   ");
	strcpy(line[63], "@@ senddiff      ");
	strcpy(line[64], "@@ fault local   ");
	strcpy(line[65], "@@ fault remote  ");

	for (i = 1; i <= 65; i++)
		if (c[i] == 0)
			printf("%2d %s = %10u %10d        N/A %10d     N/A\n", 
					i, line[i], t[i], b[i], c[i]);
		else
			printf("%2d %s= %10u %10d %10d %10d %7d\n", 
					i, line[i], t[i], b[i], s[i], c[i], t[i] / c[i] + 1);
#endif

#ifdef DOSTAT
	printf("Shared Memory (%d-byte pages) : %d (total) %d (used)\n", 
			Pagesize, Maxmemsize / Pagesize, alloctot / Pagesize);

	if (hostc > 1) {
		printf("homegrantcnt = %d\n", homegrantcnt);
		reply = newmsg();
		reply->frompid = jia_pid;
		reply->topid = 0;
		reply->size = 0;
		reply->op = STAT;
		appendmsg(reply, (char*)stat_p, sizeof(jiastat));
		appendmsg(reply, ltos(wfaultcnt), Intbytes);
		appendmsg(reply, ltos(rfaultcnt), Intbytes);
		appendmsg(reply, ltos(homegrantcnt), Intbytes);
		appendmsg(reply, ltos(pagegrantcnt), Intbytes);
		appendmsg(reply, ltos(refusecnt), Intbytes);
		appendmsg(reply, ltos(diffmsgcnt), Intbytes);
		appendmsg(reply, ltos(diffgrantmsgcnt), Intbytes);
		appendmsg(reply, ltos(dpages), Intbytes);
		appendmsg(reply, ltos(epages), Intbytes);
		appendmsg(reply, ltos(rr1), Intbytes);
		appendmsg(reply, ltos(rw1), Intbytes);
		appendmsg(reply, ltos(rr2), Intbytes);
		appendmsg(reply, ltos(rw2), Intbytes);
		appendmsg(reply, ltos(rr3), Intbytes);
		appendmsg(reply, ltos(rw3), Intbytes);
		appendmsg(reply, ltos(rr4), Intbytes);
		appendmsg(reply, ltos(rw4), Intbytes);

		waitstat = 1;
		asendmsg(reply);
		freemsg(reply);
		while(waitstat);

		if (jia_pid == 0) {
			memset((char*)&total, 0, sizeof(total)); 
			for (i = 0; i < hostc; i++) {
				total.msgsndcnt    += allstats[i].msgsndcnt;
				total.msgrcvcnt    += allstats[i].msgrcvcnt;
				total.msgsndbytes  += allstats[i].msgsndbytes;
				total.msgrcvbytes  += allstats[i].msgrcvbytes;
				total.segvLcnt     += allstats[i].segvLcnt;
				total.segvRcnt     += allstats[i].segvRcnt;
				total.barrcnt      += allstats[i].barrcnt;
				total.lockcnt      += allstats[i].lockcnt;
				total.getpcnt      += allstats[i].getpcnt;
				total.diffcnt      += allstats[i].diffcnt;
				total.invcnt       += allstats[i].invcnt;

				total.usersigiocnt += allstats[i].usersigiocnt;
				total.synsigiocnt  += allstats[i].synsigiocnt;
				total.segvsigiocnt += allstats[i].segvsigiocnt;

				total.segvLtime    += allstats[i].segvLtime;
				total.segvRtime    += allstats[i].segvRtime;
				total.barrtime     += allstats[i].barrtime;
				total.locktime     += allstats[i].locktime;
				total.unlocktime   += allstats[i].unlocktime;
				total.usersigiotime += allstats[i].usersigiotime;
				total.synsigiotime  += allstats[i].synsigiotime;
				total.segvsigiotime += allstats[i].segvsigiotime;

				total.syntime     += allstats[i].syntime;
				total.datatime    += allstats[i].datatime;
				total.othertime   += allstats[i].othertime;
				total.segvtime    += allstats[i].segvtime;
				total.commtime    += allstats[i].commtime;
				total.difftime    += allstats[i].difftime;
			}	

			for (i = 0; i < hostc * 9 / 2 - 1; i++) printf("#");
			printf("  JUMP STATISTICS  ");
			for (i = 0; i < hostc * 9 / 2; i++) printf("#");
			if (hostc % 2) printf("#");
			printf("\n           hosts --> ");
			for (i = 0; i < hostc; i++) printf("%8d ", i);
			printf("\n");
			for (i = 0; i < 20 + hostc * 9; i++) printf("-");
			printf("\nMsgs Sent          = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].msgsndcnt);
			printf("\nMsgs Received      = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].msgrcvcnt);
			printf("\nBytes Sent         = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].msgsndbytes);
			printf("\nBytes Received     = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].msgrcvbytes);
			printf("\nSEGVs (local)      = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].segvLcnt);
			printf("\nSEGVs (remote)     = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].segvRcnt);
			printf("\nSIGIOs(user)       = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].usersigiocnt);
			printf("\nSyn. SIGIOs        = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].synsigiocnt);
			printf("\nSEGV SIGIOs        = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].segvsigiocnt);
			printf("\nBarriers           = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].barrcnt);
			printf("\nLocks              = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].lockcnt);
			printf("\nGetp Reqs          = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].getpcnt);
			printf("\nDiffs              = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].diffcnt);
			printf("\nInvalidate         = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].invcnt);
			printf("\nMWdiff             = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].mwdiffcnt);
			printf("\nSWdiff             = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allstats[i].swdiffcnt);

			printf("\n");
			for (i = 0; i < 20 + hostc * 9; i++) printf("-");
			printf("\nST01 Barrier time       = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].barrtime / 1000000.0);
			printf("\nST01 Lock time          = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].locktime / 1000000.0);
			printf("\nST01 Unlock time        = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].unlocktime / 1000000.0);
			printf("\nST01 SEGV time(local)   = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].segvLtime / 1000000.0);
			printf("\nST01 SEGV time(remote)  = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].segvRtime / 1000000.0);
			printf("\nST01 SIGIO time(user)   = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].usersigiotime / 1000000.0);
			printf("\nST01 Syn. SIGIO time    = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].synsigiotime / 1000000.0);
			printf("\nST01 SEGV SIGIO time    = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].segvsigiotime / 1000000.0);
			printf("\nST01 Encode diff time   = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].endifftime / 1000000.0);
			printf("\nST01 Decode diff time   = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].dedifftime / 1000000.0);
			printf("\nST01 Asendmsg time      = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", allstats[i].asendtime / 1000000.0);

			printf("\n");
			for (i = 0; i < 20 + hostc * 9; i++) printf("-");

			printf("\n* SEGV time          = ");
			for (i = 0; i < hostc; i++) 
				printf("%8.2f ", (allstats[i].segvLtime + allstats[i].segvRtime
							+ allstats[i].segvLcnt * SEGVoverhead
							+ allstats[i].segvRcnt * SEGVoverhead
							- allstats[i].segvsigiotime - allstats[i].segvsigiocnt
							* SIGIOoverhead) / 1000000.0);
			printf("\n* Syn. time          = ");
			for (i=0; i<hostc; i++) 
				printf("%8.2f ", (allstats[i].barrtime + allstats[i].locktime
							+ allstats[i].unlocktime - allstats[i].synsigiotime
							- allstats[i].synsigiocnt * SIGIOoverhead) / 1000000.0);
			printf("\n* Server time        = ");
			for (i=0; i<hostc; i++)
				printf("%8.2f ", (allstats[i].usersigiotime
							+ allstats[i].synsigiotime + allstats[i].segvsigiotime
							+ (allstats[i].usersigiocnt + allstats[i].synsigiocnt
								+ allstats[i].segvsigiocnt) * SIGIOoverhead) / 1000000.0);
			printf("\n");
			for (i = 0; i < 20 + hostc * 9; i++) printf("#");
			printf("\n");

			for (i = 0; i < hostc * 9 / 2 - 1; i++) printf("#");
			printf("  NEW STATISTICS  ");
			for (i = 0; i < hostc * 9 / 2; i++) printf("#");
			if (hostc % 2) printf("#");
			printf("\n           hosts --> ");
			for (i = 0; i < hostc; i++) printf("%8d ", i);
			printf("\n");
			for (i = 0; i < 20 + hostc * 9; i++) printf("-");
			printf("\n* Write Faults       = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][0]);
			printf("\n* Read Faults        = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][1]);
			printf("\n* Home Grants        = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][2]);
			printf("\n* Page Grants        = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][3]);
			printf("\n* Refuses            = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][4]);
			printf("\n* Diff Msg Recv      = ");
			for (i = 0; i<hostc; i++) printf("%8d ", allnew[i][5]);
			printf("\n* DiffGrant Msg Recv = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][6]);
			printf("\n* Diffed Pages       = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][7]);
			printf("\n* Empty Diffed Pages = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][8]);
			printf("\n* R fault 1    = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][9]);
			printf("\n* W fault 1    = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][10]);
			printf("\n* R fault 2    = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][11]);
			printf("\n* W fault 2    = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][12]);
			printf("\n* R fault 3    = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][13]);
			printf("\n* W fault 3    = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][14]);
			printf("\n* R fault 4    = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][15]);
			printf("\n* W fault 4    = ");
			for (i = 0; i < hostc; i++) printf("%8d ", allnew[i][16]);
			printf("\n");
			for (i = 0; i < 20+hostc*9; i++) printf("#");
			printf("\n");
			for (i = 0; i < 20+hostc*9; i++) printf("#");
			printf("\n");

			totalmsg = totalbyte = totalloc = totalrr1 = totalrr2 = 0;
			totalrr3 = totalrr4 = totalrw1 = totalrw2 = totalrw3 = 0;
			totalrw4 = totaldmsg = totaldiff = totalediff = totalgrant = 0;

			for (i = 0; i < hostc; i++)
			{
				totalmsg += allstats[i].msgrcvcnt;
				if (allstats[i].msgrcvbytes % 100 >= 50)
					totalbyte += ((allstats[i].msgrcvbytes / 100) + 1);
				else
					totalbyte += (allstats[i].msgrcvbytes / 100);
				totalloc += allstats[i].segvLcnt;
				totalrr1 += allnew[i][9];
				totalrw1 += allnew[i][10];
				totalrr2 += allnew[i][11];
				totalrw2 += allnew[i][12];
				totalrr3 += allnew[i][13];
				totalrw3 += allnew[i][14];
				totalrr4 += allnew[i][15];
				totalrw4 += allnew[i][16];
				totaldmsg += allnew[i][5];
				totaldiff += allnew[i][7];
				totalediff += allnew[i][8];
				totalgrant += allnew[i][2];
			}
			if (totalbyte % 10 >= 5)
				totalbyte = totalbyte / 10 + 1;
			else
				totalbyte = totalbyte / 10;
			totalpf1  = totalloc;
			totalpf2  = totalrr2 + totalrw2 + totalrr4 + totalrw4; 
			totalpf3d = totalediff;
			totalpf3c = 0;
			totalpf3b = totaldiff - totalpf2;
			totalpf3a = totalrr1 + totalrw1 + totalrr3 + totalrw3 -
				totalpf3d - totalpf3b;

			printf("TTLSTAT-MSG   = %d\n", totalmsg);
			printf("TTLSTAT-KBYTE = %d\n", totalbyte);
			printf("TTLSTAT-LOCAL = %d\n", totalloc);
			printf("TTLSTAT-RR1 = %d\n", totalrr1);
			printf("TTLSTAT-RW1 = %d\n", totalrw1);
			printf("TTLSTAT-RR2 = %d\n", totalrr2);
			printf("TTLSTAT-RW2 = %d\n", totalrw2);
			printf("TTLSTAT-RR3 = %d\n", totalrr3);
			printf("TTLSTAT-RW3 = %d\n", totalrw3);
			printf("TTLSTAT-RR4 = %d\n", totalrr4);
			printf("TTLSTAT-RW4 = %d\n", totalrw4);
			printf("TTLSTAT-DMSG = %d\n", totaldmsg);
			printf("TTLSTAT-DIFF = %d\n", totaldiff);
			printf("TTLSTAT-EDIFF = %d\n", totalediff);
			printf("TTLSTAT-GRANT = %d\n", totalgrant);
			printf("TTLSTAT-PF1 = %d\n", totalpf1);
			printf("TTLSTAT-PF2 = %d\n", totalpf2);
			printf("TTLSTAT-PF3A = %d\n", totalpf3a);
			printf("TTLSTAT-PF3B = %d\n", totalpf3b);
			printf("TTLSTAT-PF3C = %d\n", totalpf3c);
			printf("TTLSTAT-PF3D = %d\n", totalpf3d);
		}
	}
#else  /* DOSTAT */
	jia_wait();
#endif /* DOSTAT */

	for (i = 0; i < hostc; i++)
	{
		close(commreq.snd_fds[i]);
		close(commreq.rcv_fds[i]);
		close(commrep.snd_fds[i]);
		close(commrep.rcv_fds[i]);
	}
}

#else  /* NULL_LIB */

void jia_exit()
{
	exit(0);
} 

#endif /* NULL_LIB */
