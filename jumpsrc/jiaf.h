C***********************************************************************
C                                                                      *
C    The JIAJIA Software Distributed Shared Memory System              * 
C                                                                      *
C    Copyright (C) 1997 the Center of High Performance Computing       * 
C    of Institute of Computing Technology, Chinese Academy of          * 
C    Sciences.  All rights reserved.                                   * 
C                                                                      *
C    Permission to use, copy, modify and distribute this software      * 
C    is hereby granted provided that (1) source code retains these     * 
C    copyright, permission, and disclaimer notices, and (2) redistri-  *
C    butions including binaries reproduce the notices in supporting    *
C    documentation, and (3) all advertising materials mentioning       * 
C    features or use of this software display the following            * 
C    acknowledgement: ``This product includes software developed by    *
C    the Center of High Performance Computing, Institute of Computing  *
C    Technology, Chinese Academy of Sciences."                         * 
C                                                                      *
C    This program is distributed in the hope that it will be useful,   *
C    but WITHOUT ANY WARRANTY; without even the implied warranty of    *
C    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              * 
C                                                                      *
C    Center of High Performance Computing requests users of this       * 
C    software to return to dsm@water.chpc.ict.ac.cn any                * 
C    improvements that they make and grant CHPC redistribution rights. *
C                                                                      *
C           Author: Weiwu Hu, Weisong Shi, Zhimin Tang                 * 
C                                                                      *
C***********************************************************************


	external jia_init !$pragma C(jia_init)
	external jia_alloc3 !$pragma C(jia_alloc3)
	external jia_alloc2 !$pragma C(jia_alloc2)
	external jia_alloc !$pragma C(jia_alloc)
	external jia_lock !$pragma C(jia_lock)
	external jia_unlock !$pragma C(jia_unlock)
	external jia_barrier !$pragma C(jia_barrier)
	external jia_wait !$pragma C(jia_wait)
	external jia_exit !$pragma C(jia_exit)
	external jia_error !$pragma C(jia_error)
	external jia_clock !$pragma C(jia_clock)
	real     jia_clock
c	external getpid,gethosts 
c	integer  getpid, gethosts
	common /jia/ jiapid, jiahosts
