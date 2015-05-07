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
 *          Author: Weiwu Hu, Weisong Shi, Zhimin Tang                 * 
 * =================================================================== *
 *   This software is ported to SP2 by                                 *
 *                                                                     *
 *         M. Rasit Eskicioglu                                         *
 *         University of Alberta                                       *
 *         Dept. of Computing Science                                  *
 *         Edmonton, Alberta T6G 2H1 CANADA                            *
 * =================================================================== *
 * $Log: syn.h,v $
 * Revision 1.0  1998/02/19 06:35:20  rasit
 * Initial revision
 *
 **********************************************************************/

#ifndef JIASYN_H
#define JIASYN_H

#define Maxwtnts     511
#define Maxstacksize 5
#define hidelock     (Maxlocks + Maxcondvs)
#define top          lockstack[stackptr]
#define stol(x)      (*((unsigned long *)(x)))
#define ltos(x)      ((unsigned char *)(&(x)))
#define sbit(s,n)    ((s[(n)/8]) |= ((unsigned char)(0x1 << ((n) % 8))))
#define cbit(s,n)    ((s[(n)/8]) &= (~((unsigned char)(0x1 << ((n) % 8)))))
#define tbit(s,n)    (((s[(n)/8]) & ((unsigned char)(0x1 << ((n) % 8)))) != 0) 
#define WNULL        ((wtnt_t*) NULL)


typedef struct wtnttype {
	int	        wtnts[Maxwtnts];
	int             from[Maxwtnts];
	int             wtntc;
	struct wtnttype *more;
} wtnt_t;

typedef struct locktype {
	int         acqs[Maxhosts];
	int         acqc;
	int         condv;
	wtnt_t      *wtntp;
} jialock_t;

typedef struct stacktype {
	int         lockid;
	wtnt_t      *wtntp;
} jiastack_t;

#endif /* JIASYN_H */
