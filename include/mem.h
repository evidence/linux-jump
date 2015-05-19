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
 * $Log: mem.h,v $
 * Revision 1.1  1998/03/06 05:18:11  dsm
 * no log.
 *
 * Revision 1.0  1998/02/19 18:34:02  rasit
 * Initial revision
 *
 **********************************************************************/

#ifndef JIAMEM_H
#define JIAMEM_H

#define   RESERVE_TWIN_SPACE
#define   REPSCHEME   0
#define   Maxdiffs    64
#define   Diffunit    4
#define   Dirtysize   (Pagesize / (Diffunit * 8))

#define   Homepages   Maxmempages
#define   Homesize    (Homepages * Pagesize)
#define   Cachesize   (Pagesize * Cachepages) 
#define   Setpages    Cachepages
#define   Setnum      (Cachepages / Setpages)

#define   LENGTH      ((Maxmempages + 31) / 32)
#define   CHARBITS    32

#define   LOCKLEN     ((Maxlocks + CHARBITS - 1) / CHARBITS)

#define   DIFFNULL  ((jia_msg_t*) NULL)

#define   homehost(addr) page[((unsigned long)(addr)-Startaddr)/Pagesize].homepid
#define   homepage(addr) ((unsigned long)(addr)-Startaddr)/Pagesize

#define   memprotect(argx, argy, argz)                               	\
{									\
	int protyes;                                            	\
	unsigned long ad;                                       	\
	protyes=mprotect((caddr_t)(argx),(size_t)(argy),argz);  	\
	if (protyes != 0) {                                        	\
		ad=(unsigned long)argx;                               	\
		printf("mprotect() called from file %s line %d\n",__FILE__, __LINE__); \
		sprintf(errstr,"mprotect failed! addr=0x%lx,errno=%d",ad,errno); \
		assert((protyes==0),errstr);                          	\
	}                                                       	\
}

#define   memmap(arg1,arg2,arg3,arg4,arg5,arg6)							\
{												\
	caddr_t mapad;										\
	mapad=mmap((caddr_t)(arg1),(size_t)(arg2),(arg3),(arg4),(arg5),(arg6));			\
	if (mapad != (caddr_t)(arg1)) {								\
		sprintf(errstr,"mmap failed! addr=0x%lx, errno=%d",(unsigned long)(arg1),errno);\
		assert(0,errstr);								\
	}											\
}

typedef unsigned char* address_t; 

typedef enum { UNMAP, INV, RO, RW, R1, MAP } pagestate_t;

typedef struct {
	address_t          addr;
	unsigned short int cachei;
	unsigned short int state;
	char               wtnt;  
	char               rdnt;
	char               homepid;
	char               oldhome;
	char               pend[Maxhosts];
} jiapage_t;

typedef struct {
	pagestate_t        state;
	address_t          addr;
	address_t          twin;
	char               wtnt;   
} jiacache_t;

#endif /* JIAMEM_H */
