#ifndef JIAMEM_H
#define JIAMEM_H

#include "assert.h"

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
#define   pageaddr(addr) ((unsigned long)(addr) & ~(Pagesize - 1))

#define   memprotect(argx, argy, argz)							\
{											\
	int protyes;									\
	unsigned long ad;								\
	protyes=mprotect((caddr_t)(argx),(size_t)(argy),argz);				\
	if (protyes != 0) {								\
		ad=(unsigned long)argx;							\
		printf("mprotect() called from file %s line %d\n",__FILE__, __LINE__);	\
		RASSERT((protyes==0), "mprotect failed! addr=0x%lx,errno=%d",ad,errno); \
	}										\
}

#define   memmap(arg1,arg2,arg3,arg4,arg5,arg6)							\
{												\
	caddr_t mapad;										\
	mapad=mmap((caddr_t)(arg1),(size_t)(arg2),(arg3),(arg4),(arg5),(arg6));			\
	if (mapad != (caddr_t)(arg1))								\
		RASSERT(0, "mmap failed! addr=0x%lx, mapad=0x%lx, size=%d, fd=%ld, errno=%d",\
				(unsigned long)(arg1), (unsigned long)(mapad), arg2, arg5, errno);\
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
