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
 * $Log: init.h,v $
 * Revision 1.1  1998/03/06 05:14:42  dsm
 * add state .
 *
 * Revision 1.1  1998/03/05 07:43:19  rasit
 * New variables for stats
 *
 * Revision 1.0  1998/02/19 18:34:02  rasit
 * Initial revision
 *
 **********************************************************************/

#ifndef JIACREAT_H
#define JIACREAT_H

static char rcsid_init_h[] = "$Id: init.h,v 1.1 1998/03/06 05:14:42 dsm Exp $";

#define   Wordsize    80
#define   Linesize    400
#define   Wordnum     3
#define   Maxwords    10
#define   Maxfileno   1024

#define   SEGVoverhead   600
#define   SIGIOoverhead  200

typedef struct host_t {
  char name[Wordsize];
  char user[Wordsize];
  char dir[Wordsize];
  char passwd[Wordsize];
  char addr[Wordsize];             /* IP address */
  int  addrlen;
  int  homesize;
  int  homestart;
  int  riofd;
  int  rerrfd;
} host_t;

#ifdef DOSTAT
typedef struct Stats {
        unsigned int kernelflag;

        unsigned int msgsndbytes;
        unsigned int msgrcvbytes;
        unsigned int msgrcvcnt;
        unsigned int msgsndcnt;
        unsigned int segvLcnt;
        unsigned int segvRcnt;
        unsigned int usersigiocnt;
        unsigned int synsigiocnt;
        unsigned int segvsigiocnt;
        unsigned int barrcnt;
        unsigned int lockcnt;
        unsigned int getpcnt;
        unsigned int diffcnt;
        unsigned int invcnt;
        unsigned int mwdiffcnt;
        unsigned int swdiffcnt;

        unsigned int segvLtime;
        unsigned int segvRtime;
        unsigned int barrtime;
        unsigned int locktime;
        unsigned int unlocktime;
        unsigned int usersigiotime;
        unsigned int synsigiotime;
        unsigned int segvsigiotime;
        
        unsigned int asendtime;
        unsigned int endifftime;
        unsigned int dedifftime;

        /*Follow used by Shi*/
        unsigned int difftime;
	unsigned int busytime;
	unsigned int datatime;
	unsigned int syntime;
	unsigned int othertime;
	unsigned int segvtime;
	unsigned int commtime;

} jiastat_t;

#endif

#endif /* JIACREAT_H */
