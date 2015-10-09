#ifndef NULL_LIB
#include <ucontext.h>
#include "global.h"
#include "mem.h"
#include "init.h"
#include "comm.h"
#include "syn.h"
#include "assert.h"

extern jia_msg_t *newmsg();
extern void freemsg();
extern void asendmsg(jia_msg_t *req);
extern void savepage(int cachei);
extern address_t newtwin();
extern void freetwin(address_t twin);
extern void enable_sigio();
extern void disable_sigio_sigalrm();
extern void appendmsg(jia_msg_t *msg, unsigned char *str, int len);

int findposition(address_t addr);
unsigned char temppage[Pagesize];
volatile int mapped, tempcopy = 0;

mem_llist_t	mem_llist1, mem_llist2;
cache_llist_t	cache_llist;

unsigned long SpecialAddr;
unsigned int oaccess[Maxlocks][LENGTH];

extern int jia_pid;
extern host_t hosts[Maxhosts];
extern int hostc;
extern jiastack_t lockstack[Maxstacksize];
extern int stackptr;

#ifdef DOSTAT
extern jiastat_t jiastat;
int wfaultcnt;
int rfaultcnt;
int homegrantcnt;
int pagegrantcnt;
int refusecnt;
int diffmsgcnt;
int diffgrantmsgcnt;
int dpages;
int epages;
unsigned int rr1, rr2, rr3, rr4, rw1, rw2, rw3, rw4;
#endif

#ifdef JT
extern int b[70], c[70], s[70], t[70];
#endif

jiacache_t cache[Cachepages+1];
jiapage_t page[Maxmempages+1];

unsigned long globaladdr;
int numberofpages;
int value;
unsigned long totalhome;

long jiamapfd;

volatile int getpwait;
volatile int diffwait;
int repcnt[Setnum];

int alloctot;
jia_msg_t *diffmsg[Maxhosts];

void getpage(address_t addr)
{
	int homeid;
	jia_msg_t *req;
	int pagei, i, j, k, flag;

	homeid = homehost(addr);

#ifdef MHPDEBUG
	RASSERT((homeid != jia_pid), "This should not have happened (2)");
#endif

	req = newmsg();
	req->op = GETP;
	req->frompid = jia_pid;
	req->topid = homeid;
	req->size = 0;
	appendmsg(req, ltos(addr), Longbytes);
	int one = 1;
	appendmsg(req, ltos(one), Intbytes);

	/* if one of the locks held by the requesting proc has accessed the */
	/* required page before without propagating the diffs, we shouldn't */
	/* get the copy from the prev home -- hence flag should be set to 1 */
	flag = 0;
	pagei = homepage(addr);
	for (i = 1; i <= stackptr && flag == 0; i++) {
		j = lockstack[i].lockid;
		k = oaccess[j][pagei / CHARBITS];
		if ((k >> (pagei % CHARBITS)) % 2 == 1)
			flag = 1;
	}
	appendmsg(req, ltos(flag), Intbytes);

	getpwait = 1;
	asendmsg(req);
	freemsg(req);
}


/**
 * Handler for the SIGSEGV signal.
 */
void sigsegv_handler (int sig, siginfo_t *sip, void *context)
{
	address_t faultaddr;
	int writefault, cachei, temp, flaggy;
	unsigned long faultpage;
	sigset_t set, oldset;
	ucontext_t *uctx = (ucontext_t *) context;
	if (sig != SIGSEGV)
		fprintf(stderr, "ERROR: sigsegv_handler received signal %d\n", sig);
	if ((sip == NULL) || (context == NULL))
		fprintf(stderr, "ERROR: NULL parameters to sigsegv_handler!");

#ifdef DOSTAT
	register unsigned int begin;
#endif

#ifdef JT
	register unsigned int x1, x2, x3;
	x1 = get_usecs();
#endif

	/* We should not allow any other events to interrupt SIGSEGV.
	 * Therefore, we block the SIGALRM signal.*/
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, &oldset);

#ifdef DOSTAT
	begin = get_usecs();
	jiastat.kernelflag = 2;
#endif

#ifdef JT
	x3 = 0;
#endif

	/* We unblock the SIGIO signal: */
	sigemptyset(&set);
	sigaddset(&set, SIGIO);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

	faultaddr = (address_t) pageaddr(sip->si_addr);
	dprintf("faultaddr=0x%lx, pageaddr=0x%lx", (unsigned long) sip->si_addr, (unsigned long) faultaddr);
	/* We check the error register in mcontext_t.
	   It is hardware-dependent (see /usr/include/sys/ucontext.h) */
#ifdef ARCH_X86
	writefault = (((unsigned int)uctx->uc_mcontext.gregs[REG_ERR]) & 0x2);
#elif defined ARCH_ARM
	writefault = (((unsigned int)uctx->uc_mcontext.error_code & (1<<11)) >> 11);
#else
	#error "No architecture specified!"
#endif

	faultpage = (unsigned long) homepage(faultaddr);
	RASSERT((((unsigned long)faultaddr < (Startaddr + globaladdr)) && 
				((unsigned long)faultaddr >= Startaddr)), 
				"Access shared memory out of range from 0x%x to 0x%lx!, faultaddr = 0x%lx, writefault = 0x%x", Startaddr,
			Startaddr + globaladdr, (unsigned long int) faultaddr, writefault);


#ifdef DOSTAT
	if (writefault)
		wfaultcnt++;
	else 
		rfaultcnt++;
#endif

	disable_sigio_sigalrm();  /* We should not let SIGIO interrupt again */

	if (homehost(faultaddr) == jia_pid) {
		/* Local SEGV -- RW request on Read-only home page */
#ifdef DOSTAT
		jiastat.segvLcnt++;
#endif

		memprotect((caddr_t)faultaddr, Pagesize, PROT_READ | PROT_WRITE);
		if (page[faultpage].wtnt == 0) {
			page[faultpage].wtnt = 1;
			insert(mem_llist2, faultpage);
		}
		temp = page[faultpage].oldhome;

		if (temp == Maxhosts) {
			if (!writefault) {
				RASSERT((0 == 1), "Unexpected error 5-1!\n");
			} else {

#ifdef MHPDEBUG
				if (!(page[faultpage].state == RO &&
							page[faultpage].pend[jia_pid] == 0))
					RASSERT((0 == 1), "Error 5-2: Page %d, state %d (RO), pend[%d] = %d (0).\n",
							faultpage, page[faultpage].state,
							jia_pid, page[faultpage].pend[jia_pid]);

#endif

				if (page[faultpage].pend[jia_pid] == 0) {
					page[faultpage].pend[jia_pid]++;
					insert(mem_llist1, faultpage);
				} else {
					printf("Logic bug: Page %ld already pended %d\n", faultpage,
							page[faultpage].pend[jia_pid]);
				}
			}
		}
		page[faultpage].state = RW;

		enable_sigio();

#ifdef DOSTAT
		jiastat.segvLtime += get_usecs() - begin;
		jiastat.kernelflag = 0;
#endif

	} else {   /* Non-local Page-Fault */
		enable_sigio();

#ifdef DOSTAT
		jiastat.segvRcnt++;
#endif

		/* Find if page is cached */
		cachei = (int)page[((unsigned long)faultaddr - Startaddr)
			/ Pagesize].cachei;
		if (cachei < Cachepages) {   /* Found in cache */
			memprotect((caddr_t)faultaddr, Pagesize, PROT_READ | PROT_WRITE);

			if (!((writefault) && (cache[cachei].state == RO))) {
				/* If it is a writefault and cache copy is read-only */
				/* we should not ask the protocol to use the cache space */
				mapped = 1;

#ifdef DOSTAT
				if (writefault)
					rw1++; 
				else 
					rr1++;
#endif
#ifdef JT
				x3 = 1;
#endif
				getpage(faultaddr);
			} else {

#ifdef DOSTAT
				if (writefault) 
					rw2++; 
				else 
					rr2++;
#endif
				tempcopy = 0;
			}
		} else {
			if (page[faultpage].state == UNMAP) {
				mapped = 0;
			} else { 
				mapped = 1;
			}
			if (mapped == 0) {
				memmap(faultaddr, Pagesize, PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_FIXED, jiamapfd, 0);
			} else {
				memprotect((caddr_t)faultaddr, (size_t)Pagesize,
						PROT_READ | PROT_WRITE);
			}
			mapped = 1;

			disable_sigio_sigalrm();

			temp = page[faultpage].oldhome;
			if (!((page[faultpage].state == RO) && (temp == jia_pid))) {

#ifdef DOSTAT
				if (writefault)
					rw3++;
				else
					rr3++; 
#endif
				enable_sigio();

#ifdef JT
				x3 = 1;
#endif
				getpage(faultaddr);
			} else {
				disable_sigio_sigalrm();
				page[faultpage].pend[jia_pid] = 1;
				insert(mem_llist1, faultpage);
				enable_sigio();

#ifdef MHPDEBUG
				if (!(page[faultpage].pend[jia_pid] == 1 &&
							page[faultpage].oldhome == jia_pid && writefault))
					RASSERT((0 == 1), "Error: page[%d].pend[%d] = %d, oldhome %d (%d), wf %d.\n",
							faultpage, jia_pid, 
							page[faultpage].pend[jia_pid],
							page[faultpage].oldhome, jia_pid, writefault);

#endif

#ifdef DOSTAT
				if (writefault)
					rw4++;
				else
					rr4++;
#endif
				enable_sigio();
			}
			cachei = findposition(faultaddr);
		}

		if (writefault) {
			cache[cachei].addr = faultaddr;
			cache[cachei].state = RW;
			while(getpwait);    /* wait page to come */

			if (homehost(faultaddr) == jia_pid) {
				cache[cachei].addr = 0;   /* page become home, no need to cache */
				disable_sigio_sigalrm();
				if (cache[cachei].wtnt == 1) {
					cache[cachei].wtnt = 0;
					dislink(cache_llist, cachei);
				}
				enable_sigio();

				cache[cachei].state = UNMAP;
				page[faultpage].cachei = Cachepages;
				page[faultpage].state = RW;

				disable_sigio_sigalrm();
				if (page[faultpage].wtnt == 0) {
					page[faultpage].wtnt = 1;
					insert(mem_llist2, faultpage);
				}
				enable_sigio();
				flaggy = 0;
			} else {
				disable_sigio_sigalrm();
				if (cache[cachei].wtnt == 0) {
					cache[cachei].wtnt = 1;
					insert(cache_llist, cachei);
				}
				enable_sigio();
				flaggy = 1;
			}
			/* create twin */
			if (homehost(faultaddr) != jia_pid)
				cache[cachei].twin = newtwin();
			/* bring page content from temp buffer */
			if (tempcopy == 1)
				memcpy(faultaddr, temppage, Pagesize);
			/* duplicate the twin */
			if (homehost(faultaddr) != jia_pid)
				memcpy(cache[cachei].twin, faultaddr, Pagesize);
		} else {   /* a read fault */
			cache[cachei].addr = faultaddr;
			cache[cachei].state = RO;
			/* wait for page to come */
			while(getpwait);
			if (homehost(faultaddr) == jia_pid) {
				cache[cachei].addr = 0;

				disable_sigio_sigalrm();
				if (cache[cachei].wtnt == 1) {
					cache[cachei].wtnt = 0;
					dislink(cache_llist, cachei);
				}
				enable_sigio();

				cache[cachei].state = UNMAP;
				page[faultpage].cachei = Cachepages;
				page[faultpage].state = RO;
				flaggy = 0;
			} else {
				disable_sigio_sigalrm();
				if (cache[cachei].wtnt == 0) {
					cache[cachei].wtnt = 1;
					insert(cache_llist, cachei);
				}
				enable_sigio();
				flaggy = 1;
			}
			if (tempcopy == 1)
				memcpy(faultaddr, temppage, Pagesize);
			memprotect((caddr_t)faultaddr, (size_t)Pagesize, PROT_READ);
			if (flaggy == 1)
				page[faultpage].state = MAP; 
		}

#ifdef DOSTAT
		jiastat.segvRtime += get_usecs() - begin;
		jiastat.kernelflag = 0;
#endif
	}

#ifdef JT
	x2 = get_usecs();
	t[64 + x3] += (x2 - x1);
	if (x2 - x1 > b[64 + x3])
		b[64 + x3] = x2 - x1;
	if (x2 - x1 < s[64 + x3])
		s[64 + x3] = x2 - x1;
	c[64 + x3]++;
#endif

	sigprocmask(SIG_SETMASK, &oldset, NULL);
}


/* Initialization routine */
void initmem()
{
	int i,j;

	for (i = 0; i < Maxhosts; i++) {
		diffmsg[i] = DIFFNULL;
	}
	diffwait = 0;
	getpwait = 0;
	SpecialAddr = 0x68000000;
	/* SpecialAddr specifies the start of migration notices */

	numberofpages = 0;
	mem_llist1.head = -1;
	mem_llist1.tail = -1;
	mem_llist2.head = -1;
	mem_llist2.tail = -1;
	cache_llist.head = -1;
	cache_llist.tail = -1;


	for (i = 0; i <= hostc; i++) {
		hosts[i].homesize = 0;
	}

	for (i = 0; i <= (Maxmempages); i++) {
		page[i].wtnt = 0;
		page[i].rdnt = 0;
		page[i].addr = (address_t)0;
	}

	for (i = 0; i < Maxmempages; i++) {
		page[i].cachei = (unsigned short)Cachepages;  /* means uncached */
		page[i].homepid = (unsigned short)Maxhosts;
	}

	for (i = 0; i < Maxlocks; i++)
		for (j = 0; j < LENGTH; j++)
			oaccess[i][j] = 0;

	for (i = 0; i <= Cachepages; i++) {
		cache[i].state = UNMAP;
		cache[i].addr = 0;
		cache[i].twin = NULL;
		cache[i].wtnt = 0;
	}

	globaladdr = 0;

	jiamapfd = open("/dev/zero", O_RDWR,0);
	{
		struct sigaction act;
		act.sa_handler = (void_func_handler)sigsegv_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_NODEFER|SA_SIGINFO;
		if (sigaction(SIGSEGV, &act, NULL))
			ASSERT(0,"segv sigaction problem");
	}

	for (i = 0; i < Setnum; i++)
		repcnt[i] = 0;
	srand(1);

#ifdef DOSTAT
	rr1 = rr2 = rr3 = rr4 = rw1 = rw2 = rw3 = rw4 = 0;
	wfaultcnt = 0;
	rfaultcnt = 0;
	homegrantcnt = 0;
	pagegrantcnt = 0;
	refusecnt = 0;
	diffmsgcnt = 0;
	diffgrantmsgcnt = 0;
	dpages = 0;
	epages = 0;
#endif
}

unsigned long jia_alloc3(int size, int block, int starthost)
{
	int homepid;
	int mapsize;
	int allocsize;
	unsigned long originaddr;
	int pagei, i, j;

	if (!((globaladdr + size) <= Maxmemsize))
		RASSERT((0 == 1), "Insufficient shared space! Max = 0x%x Left = 0x%lx Need = 0x%x\n",
				Maxmemsize, Maxmemsize - globaladdr,size);

	originaddr = globaladdr;
	allocsize = ((size % Pagesize) == 0) ? (size) 
		: ((size / Pagesize + 1) * Pagesize); 
	mapsize = ((block % Pagesize) == 0) ? (block)
		: ((block / Pagesize + 1) * Pagesize); 
	homepid = starthost;

	dprintf("size=%d, block=%d, starthost=%d, globaladdr=%lu",
			size, block, starthost, globaladdr);

	while(allocsize > 0) {
		if (jia_pid == homepid) {
			RASSERT((hosts[homepid].homesize + mapsize) < (Maxmempages * Pagesize), "Too many home pages");
			if (hostc > 1) {
				memmap(Startaddr + globaladdr, mapsize, PROT_READ, 
						MAP_PRIVATE | MAP_FIXED, jiamapfd, 0);
			} else {
				memmap(Startaddr + globaladdr, mapsize, PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_FIXED, jiamapfd, 0);
			}
		}

		for (i = 0; i < mapsize; i += Pagesize) {
			pagei = (globaladdr + i) / Pagesize;
			page[pagei].homepid = homepid;
			page[pagei].addr = (address_t)(Startaddr + globaladdr + i);
			if (homepid != jia_pid)
				page[pagei].state = UNMAP;
			else
				page[pagei].state = RO;

			page[pagei].oldhome = Maxhosts;   /* no prev home at this moment */
			for (j = 0; j < Maxhosts; j++)
				page[pagei].pend[j] = 0;
			numberofpages++;
		}

		hosts[homepid].homesize += mapsize;
		globaladdr += mapsize;
		allocsize -= mapsize;   
		homepid = (homepid + 1) % hostc;
	}
	return(Startaddr + originaddr);
}

unsigned long jia_alloc(int size)
{
	return(jia_alloc3(size, Pagesize, 0));
}

unsigned long jia_alloc2(int size, int block)
{
	return(jia_alloc3(size, block, 0));
}


int xor(address_t addr)
{
	return((((unsigned long)(addr - Startaddr) / Pagesize)
				% Setnum) * Setpages);
}

int replacei(int cachei)
{
	int seti;

	if (REPSCHEME == 0)
		return((random() >> 8) % Setpages);
	else {
		seti = cachei / Setpages;
		repcnt[seti] = (repcnt[seti] + 1) % Setpages;
		return(repcnt[seti]);
	} 
}

/* Flush the cached copy of a page to its home */
void flushpage(int cachei)
{
	int unmapyes;

	unmapyes = munmap((caddr_t)cache[cachei].addr, Pagesize);
	page[homepage(cache[cachei].addr)].state = UNMAP;

	RASSERT((unmapyes == 0), "munmap failed at address 0x%lx, errno = %d",
			(unsigned long)cache[cachei].addr, errno);

	page[((unsigned long)cache[cachei].addr - Startaddr)
		/ Pagesize].cachei = Cachepages;

	if (cache[cachei].state == RW) freetwin(cache[cachei].twin);
	cache[cachei].state = UNMAP;

	disable_sigio_sigalrm();
	if (cache[cachei].wtnt == 1) {
		cache[cachei].wtnt=0;
		dislink(cache_llist, cachei);
	}
	enable_sigio();

	cache[cachei].addr = 0;
}


void senddiffs()
{
	int hosti;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	for (hosti = 0; hosti < hostc; hosti++) {
		if (diffmsg[hosti] != DIFFNULL) {
			if (diffmsg[hosti]->size > 0) {
				diffwait++;
				asendmsg(diffmsg[hosti]);
			}
			freemsg(diffmsg[hosti]);
			diffmsg[hosti] = DIFFNULL;
		}
	}

#ifdef JT
	x2 = get_usecs();
	t[63] += (x2 - x1);
	if (x2 - x1 > b[63]) b[63] = x2 - x1;
	if (x2 - x1 < s[63]) s[63] = x2 - x1;
	c[63]++;
#endif
}


int findposition(address_t addr)
{
	int cachei;  /* index of cache */
	int seti;    /* index in a set */
	int invi;
	int i;

	cachei = xor(addr);
	seti = replacei(cachei);
	invi = -1;
	for (i = 0; (cache[cachei + seti].state != UNMAP) && (i < Setpages);
			i++) {
		if ((invi == (-1)) && (cache[cachei + seti].state == INV))
			invi = seti;
		seti = (seti + 1) % Setpages;
	}

	if ((cache[cachei + seti].state != UNMAP) && (invi != (-1))) {
		seti = invi;
	}   

	if ((cache[cachei + seti].state == INV)
			|| (cache[cachei + seti].state == RO)) {
		flushpage(cachei+seti);
	}
	else if (cache[cachei + seti].state == RW
			|| cache[cachei + seti].wtnt == 1) {
		savepage(cachei + seti);
		senddiffs();
		while(diffwait);
		flushpage(cachei + seti);
	}
	page[((unsigned long)addr - Startaddr) / Pagesize].cachei
		= (unsigned short)(cachei + seti);
	return(cachei + seti);
}


/* we may do this inline for faster access */
unsigned int s2i(unsigned char *str)
{
	union {
		unsigned int l;
		unsigned char c[Intbytes];
	} notype;

	notype.c[0] = str[0];
	notype.c[1] = str[1];
	notype.c[2] = str[2];
	notype.c[3] = str[3];
	return(notype.l);
}

unsigned long s2l(unsigned char *str)
{
	union {
		unsigned long l;
		unsigned char c[Longbytes];
	} notype;
	unsigned int i;

        for (i = 0; i < Longbytes; i++) {
	    notype.c[i] = str[i];
        }

	return(notype.l);
}

/* string to short int */
unsigned long s2s(unsigned char *str)
{
	union {
		unsigned short int s;
		unsigned char c[Shortbytes];
	} notype;

	notype.c[0] = str[0];
	notype.c[1] = str[1];
	return(notype.s);
}



void getpserver(jia_msg_t *req)
{
	address_t paddr; 
	jia_msg_t *rep;
	int writeflag, grant, i, from, pagei, index;
	unsigned short int count;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef MHPDEBUG
	RASSERT((req->op == GETP) && (req->topid == jia_pid), "Incorrect GETP Message!");
#endif

	disable_sigio_sigalrm();

	paddr = (address_t)stol(req->data);
	writeflag = (int)s2i(req->data + Longbytes);
	from = (int)(req->frompid);
	index = Longbytes + Intbytes; /* paddr + writeflag */

	/* calculate page ID */
	pagei = (unsigned long int)(paddr - Startaddr) / Pagesize;

	/* grant = Maxhosts means grant home to requester 		*/
	/* (0 <= grant < Maxhosts) means not grant home, but a copy 	*/ 
	/* (grant value indicates the home) 				*/
	/* (grant < 0) means refuse to send page copy			*/
	/* (grant + Maxhosts) indicates the home			*/
	grant = Maxhosts;
	count = (int)s2i(req->data + index);
	index += Intbytes;

	if (count == 1 && page[pagei].oldhome == jia_pid) {
		/* refused -- oaccess[] says page cannot be brought from prev home */
		grant = page[pagei].homepid - Maxhosts;
	}

	if (grant == Maxhosts) {
		if (page[pagei].state == INV || page[pagei].state == UNMAP) {
			grant = page[pagei].homepid - Maxhosts;
		}
		else {
			if (writeflag)
				if (page[pagei].homepid == jia_pid) {   /* I'm CURRENT home */
					if (page[pagei].oldhome != Maxhosts) {
						/* grant copy only -- prev home not sync yet */
						grant = page[pagei].homepid;
						page[pagei].rdnt = 1;

#ifdef MHPDEBUG
						if (from == page[pagei].homepid)
							printf("Error: Page request from home (code 01)\n");
#endif

						page[pagei].pend[from]++;
					}
					else {
						for (i = 0; i < hostc && grant == Maxhosts; i++)
							if (page[pagei].pend[i] != 0) {
								/* grant copy only */
								/* some proc still have a copy of page not sync */
								grant = page[pagei].homepid;
								page[pagei].rdnt = 1;

#ifdef MHPDEBUG
								if (from == page[pagei].homepid)
									printf("Error: Page request from home (code 02)\n");
#endif
								page[pagei].pend[from]++;
							}
					}
				}
				else   /* I'm prev home only, grant copy */
					if (page[pagei].oldhome == jia_pid) {
						grant = page[pagei].homepid;

#ifdef MHPDEBUG
						if (from == page[pagei].homepid)
							printf("Error: Page request from home (code 03)\n");
#endif
						page[pagei].pend[from]++;
					}
					else {  /* I'm not CURRENT or PREVIOUS home */
						grant = page[pagei].homepid - Maxhosts;   /* Refuse */
					}
			else {  /* read fault */
				grant = page[pagei].homepid;
				if (grant == jia_pid) page[pagei].rdnt = 1;
			}    
		}
	}

	if (grant == Maxhosts) {  /* Change Home Info */

#ifdef MHPDEBUG
		if (!(page[pagei].state == RO))
			RASSERT((0 == 1), "Unexpected error: page[%d].state = %d\n.",
					pagei, page[pagei].state);
#endif

		page[pagei].homepid = from;
		page[pagei].oldhome = jia_pid;
	}

#ifdef DOSTAT
	if (writeflag) {
		if (grant == Maxhosts)
			homegrantcnt++;
		else if (grant > 0)
			pagegrantcnt++;   /* STAT */
		else
			refusecnt++;
	}
#endif

	/* Reply requester */
	rep=newmsg();
	value = 0;
	rep->op=GETPGRANT;
	rep->frompid=jia_pid;
	rep->topid=req->frompid;
	rep->size=0;
	appendmsg(rep, req->data, Longbytes);
	appendmsg(rep, ltos(grant), Intbytes);
	if (grant >= 0)
		appendmsg(rep, paddr, Pagesize);
	enable_sigio();
	asendmsg(rep);
	freemsg(rep);

#ifdef JT
	x2 = get_usecs();
	t[39] += (x2 - x1);
	if (x2 - x1 > b[39]) b[39] = x2 - x1;
	if (x2 - x1 < s[39]) s[39] = x2 - x1;
	c[39]++;
#endif
}

void getpgrantserver(jia_msg_t *rep)
{
	address_t addr;
	int grant, i, j;
	unsigned int datai, pagei;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef MHPDEBUG
	RASSERT((rep->op == GETPGRANT), "Incorrect returned message!");
#endif

	disable_sigio_sigalrm();
	datai = 0;
	addr = (address_t)stol(rep->data + datai);

	/* Compute page ID */
	pagei = (unsigned long int)(addr - Startaddr) / Pagesize;

	grant = (int)s2i(rep->data + Longbytes);

	if (grant >= 0) {
		if (grant == Maxhosts) {  /* Successfully Getting the Home */
			page[pagei].homepid = jia_pid;
			page[pagei].oldhome = rep->frompid;
			if (page[pagei].pend[jia_pid] == 0) {
				page[pagei].pend[jia_pid]++;
				insert(mem_llist1, pagei);
			} else {
				printf("Flaw: Page %d already pended\n", pagei);
			}
			if (page[pagei].wtnt == 0) {
				page[pagei].wtnt = 1;
				insert(mem_llist2, pagei);
			}

			page[pagei].rdnt = 1;
			page[pagei].state = RO;
		} else {   /* Not being granted the home */
			page[pagei].homepid = grant;
		}

		datai += Longbytes; /* addr */
		datai += Intbytes; /* grant */

		/* set oaccess[] field of this page to all holding locks */ 
		for (i = 1; i <= stackptr; i++) {
			j = lockstack[i].lockid;
			if ((oaccess[j][pagei / CHARBITS] >> (pagei % CHARBITS)) % 2 == 0)
				oaccess[j][pagei / CHARBITS] += (1 << (pagei % CHARBITS));
		}

		if (mapped == 0) {   /* not mapped, put to temp buffer first */
			memcpy(temppage, rep->data + datai, Pagesize);
			tempcopy = 1;
		}
		else {  /* mapped, directly copy to page addr */
			memcpy(addr, rep->data + datai, Pagesize);
			tempcopy = 0;
		}
		enable_sigio();
		getpwait = 0;
	} else {   /* being refused to get page */
		page[pagei].homepid = grant + Maxhosts;
		enable_sigio();
		getpage(addr);   /* get page from new home again */
	}

#ifdef JT
	x2 = get_usecs();
	t[40] += (x2 - x1);
	if (x2 - x1 > b[40]) b[40] = x2 - x1;
	if (x2 - x1 < s[40]) s[40] = x2 - x1;
	c[40]++;
#endif
}

void diffserver(jia_msg_t *req)
{
	int datai;
	int pagei;
	unsigned short int i, j, k[3];
	unsigned long paddr;
	unsigned int pstop;
	unsigned int doffset;
	unsigned int dsize;
	jia_msg_t *rep;

#ifdef DOSTAT
	register unsigned int begin;
#endif

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef DOSTAT
	begin = get_usecs();
#endif

	disable_sigio_sigalrm();   /* don't disturb */

#ifdef MHPDEBUG
	RASSERT((req->op == DIFF) && (req->topid == jia_pid), "Incorrect DIFF Message!");
#endif

	rep = DIFFNULL;
	datai = 0;
	while (datai < req->size) {
		paddr = s2l(req->data + datai);
		datai += Longbytes;
		if (paddr == SpecialAddr) {   /* Start of Migration notice */
			rep = newmsg();
			rep->op = DIFFGRANT;
			rep->frompid = jia_pid;
			rep->topid = req->frompid;
			rep->size = 0;
			while (datai < req->size) {
				i = s2s(req->data + datai);
				datai += Shortbytes;
				if (i >= Maxmempages) {  /* sent from current home to prev home */
					i -= Maxmempages;
					if (page[i].state == RO) page[i].state = R1;
					for (j = 0; j < (hostc - 1) / 16 + 1; j++) {
						k[j] = s2s(req->data + datai);
						datai += Shortbytes;
					}

#ifdef MHPDEBUG
					RASSERT((page[i].oldhome == jia_pid), "Diffserver(): Not Previous Home Error!\n");
#endif

					k[0] = 0; k[1] = 0; k[2] = 0;
					for (j = 0; j < hostc; j++) {

#ifdef MHPDEBUG
						if (!(page[i].pend[j] >= 0 && page[i].pend[j] <= 1))
							RASSERT((0 == 1), "Diffserver(): page %d, pend[%d] = %d.\n",
									i, j, page[i].pend[j]);
#endif

						if (page[i].pend[j] == 1) {
							k[j/16] += (1 << (j % 16));
							disable_sigio_sigalrm();
							page[i].pend[j] = 0;
							if (j == jia_pid) {
								dislink(mem_llist1, i);
							}
							enable_sigio();
						}
					}

					page[i].oldhome = Maxhosts + 1;
					appendmsg(rep, ltos(i), Shortbytes);
					for (j = 0; j < (hostc-1) / 16 + 1; j++)
						appendmsg(rep, ltos(k[j]), Shortbytes);
					/* No need to check message size, since guaranteed to */
					/* have enough size */           
				}

				/* now can remove the oaccess info of the page */
				for (j = 0; j < Maxlocks; j++) {
					if ((oaccess[j][i / CHARBITS] >> (i % CHARBITS)) % 2)
						oaccess[j][i / CHARBITS] -= (1 << (i % CHARBITS));

#ifdef MHPDEBUG
					RASSERT(((oaccess[j][i / CHARBITS] >> (i % CHARBITS)) % 2 == 0),
							"Diffserver(): Calculation Error on oaccess[]!\n");
#endif
				}
				page[i].homepid = req->frompid;
			}
		}
		else {   /* diffs */
			pagei = homepage(paddr);   /* get page ID */

#ifdef MHPDEBUG
			if (!(page[pagei].pend[req->frompid] >= 0 &&
						page[pagei].pend[req->frompid] <= 1))
				RASSERT((0 == 1), "Diffserver() Error: page[%d].pend[%d] = %d.\n",
						pagei, req->frompid, page[pagei].pend[req->frompid]);
			if (page[pagei].homepid == req->frompid)
				printf("Error: Home won't send out diff (code 05)\n");
#endif

			/* recv diff, decrement pend flag (can become 0 or -1) */
			page[pagei].pend[req->frompid]--;
			memprotect((caddr_t)paddr, Pagesize, PROT_READ | PROT_WRITE);

			/* decode diff and apply to master copy of page */
			pstop = s2i(req->data + datai) + datai - Longbytes;
			datai += Intbytes; /* size */
			while(datai < (int) pstop) {
				dsize = s2i(req->data + datai) & 0xffff;
				doffset = (s2i(req->data + datai) >> 16) & 0xffff;
				datai += Intbytes;
				memcpy((address_t)(paddr + doffset), req->data + datai, dsize);
				datai += dsize;
			}
			if (page[pagei].wtnt != 1) {
				memprotect((caddr_t)paddr,(size_t)Pagesize,(int)PROT_READ);
			}

#ifdef DOSTAT
			jiastat.mwdiffcnt++;
#endif
		} 
	}

#ifdef DOSTAT
	jiastat.dedifftime += get_usecs() - begin;
	diffmsgcnt++;
#endif

	/* send reply message -- DIFFGRANT */
	if (rep == DIFFNULL) {
		rep = newmsg();
		rep->op = DIFFGRANT;
		rep->frompid = jia_pid;
		rep->topid = req->frompid;
		rep->size = 0;
	}

	enable_sigio();
	asendmsg(rep);
	freemsg(rep);

#ifdef JT
	x2 = get_usecs();
	t[37] += (x2 - x1);
	if (x2 - x1 > b[37]) b[37] = x2 - x1;
	if (x2 - x1 < s[37]) s[37] = x2 - x1;
	c[37]++;
#endif
}

int encodediff(int cachei, unsigned char* diff, int dflag)
{
	int size, bytei;
	unsigned int cnt, start, header;

#ifdef DOSTAT
	register unsigned int begin;
#endif

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef DOSTAT
	begin = get_usecs();
#endif

	size = 0;
	memcpy(diff + size, ltos(cache[cachei].addr), Longbytes);
	size += Longbytes;
	size += Intbytes; /* space to store the size */

	bytei = 0;

	if (dflag == 1) {
		while(bytei < Pagesize) {
			for (; (bytei<Pagesize) && (memcmp(cache[cachei].addr + bytei,
							cache[cachei].twin + bytei, Diffunit) == 0); bytei += Diffunit);
			if (bytei < Pagesize) {
				cnt = (unsigned int) 0;
				start = (unsigned int) bytei;
				for (; (bytei<Pagesize) && (memcmp(cache[cachei].addr + bytei,
								cache[cachei].twin + bytei, Diffunit) != 0); bytei += Diffunit)
					cnt += Diffunit;

				header = ((start & 0xffff) << 16) | (cnt & 0xffff);
				memcpy(diff + size, ltos(header), Intbytes);
				size += Intbytes;
				memcpy(diff + size, cache[cachei].addr + start, cnt);
				size+=cnt;   
			}
		}
	}     /* if */

	memcpy(diff + Longbytes, ltos(size), Intbytes);    /* fill size */

#ifdef DOSTAT
	jiastat.endifftime += get_usecs() - begin;
#endif

#ifdef JT
	x2 = get_usecs();
	t[60] += (x2 - x1);
	if (x2 - x1 > b[60]) b[60] = x2 - x1;
	if (x2 - x1 < s[60]) s[60] = x2 - x1;
	c[60]++;
#endif

	return(size);
}

void savediff(int cachei, int dflag)
{
	unsigned char diff[Maxmsgsize];
	int diffsize;
	int hosti;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	hosti = homehost(cache[cachei].addr);
	if (hosti != jia_pid) {
		if (diffmsg[hosti] == DIFFNULL){
			diffmsg[hosti] = newmsg();
			diffmsg[hosti]->op = DIFF; 
			diffmsg[hosti]->frompid = jia_pid; 
			diffmsg[hosti]->topid = hosti; 
			diffmsg[hosti]->size = 0;
		}
		diffsize = encodediff(cachei, diff, dflag);

#ifdef DOSTAT
		if (diffsize <= 8) epages++; else dpages++;
#endif   

		if ((diffmsg[hosti]->size + diffsize) > Maxmsgsize) {
			diffwait++;
			asendmsg(diffmsg[hosti]);
			diffmsg[hosti]->size = 0;
			appendmsg(diffmsg[hosti], diff, diffsize);
			while(diffwait);
		}
		else {
			appendmsg(diffmsg[hosti], diff, diffsize);
		}
	}

#ifdef JT
	x2 = get_usecs();
	t[61] += (x2 - x1);
	if (x2 - x1 > b[61]) b[61] = x2 - x1;
	if (x2 - x1 < s[61]) s[61] = x2 - x1;
	c[61]++;
#endif
}

void savehomechange()   /* New Function */
{
	unsigned short int j, m, count, scount;
	unsigned short int mp, zero;
	short int i;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

	count = 0;
	zero = 0;

	for (i = 0; i < hostc; i++)
		if (diffmsg[i] != DIFFNULL && 
				diffmsg[i]->size + Longbytes + 2 * Shortbytes > Maxmsgsize) {
			diffwait++;
			asendmsg(diffmsg[i]);
			diffmsg[i]->size = 0;
		}
	while(diffwait);    /* Do this Better? */

	i = mem_llist1.head;
	while (i != -1) {

#ifdef MHPDEBUG
		if (page[i].pend[jia_pid] != 1)
			printf("Error: Array 1 entry %d equals %d (code 06)\n", i,
					page[i].pend[jia_pid]);
#endif

		if (page[i].homepid == jia_pid && page[i].oldhome != Maxhosts &&
				page[i].pend[jia_pid] == 1) {
			if (count == 0) {
				for (j = 0; j < hostc; j++)
					if (j != jia_pid) {
						if (diffmsg[j] == DIFFNULL) {
							diffmsg[j] = newmsg();
							diffmsg[j]->op = DIFF;
							diffmsg[j]->frompid = jia_pid;
							diffmsg[j]->topid = j;
							diffmsg[j]->size = 0;
						}
						appendmsg(diffmsg[j], ltos(SpecialAddr), Longbytes);
					}
				count = 1;
			}
			scount = 0;
			for (j = 0; j < hostc; j++) {
				if (j != jia_pid) {
					if (diffmsg[j]->size + 2 * Shortbytes > Maxmsgsize) {
						diffwait++;
						asendmsg(diffmsg[j]);
						diffmsg[j]->size = 0;
						appendmsg(diffmsg[j], ltos(SpecialAddr), Longbytes);
						scount = 1;
					}
					if (page[i].oldhome == j) {
						mp = Maxmempages + i;
						appendmsg(diffmsg[j], ltos(mp), Shortbytes);
						for (m = 0; m < (hostc-1) / 16 + 1; m++)
							appendmsg(diffmsg[j], ltos(zero), Shortbytes);
					}
					else
						appendmsg(diffmsg[j], ltos(i), Shortbytes);
				}
			}
			if (scount) {

#ifdef MHPDEBUG
				printf("Savehomechange: I need to wait\n");
#endif
				while(diffwait);   /* Really OK ? */
			}
		}
		i = mem_llist1.pendnext[i];
	}

#ifdef JT
	x2 = get_usecs();
	t[62] += (x2 - x1);
	if (x2 - x1 > b[62]) b[62] = x2 - x1;
	if (x2 - x1 < s[62]) s[62] = x2 - x1;
	c[62]++;
#endif
}


void diffgrantserver(jia_msg_t *rep)
{
	int index;
	unsigned short int i, j, k[3], r, m;

#ifdef JT
	register unsigned int x1, x2;
	x1 = get_usecs();
#endif

#ifdef MHPDEBUG
	RASSERT((rep->op == DIFFGRANT), "Incorrect returned message!\n");
#endif

	index = 0;
	while(index < rep->size) {
		i = s2s(rep->data + index);
		for (j = 0; j < (hostc - 1) / 16 + 1; j++) {
			k[j] = s2s(rep->data + index + Shortbytes);
			index += (2 * Shortbytes);
		}
		for (j = 0; j < hostc; j++) {   /* extract the pend info */
			m = j / 16;
			r = k[m] % 2;
			k[m] = k[m] / 2;
			if (r) {
				if (j == jia_pid)
					printf("Error: Home myself should be 0 (code 07)\n");
				else
					page[i].pend[j]++;
			}
		}
		page[i].oldhome = Maxhosts;
	}

	diffwait--;

#ifdef DOSTAT
	diffgrantmsgcnt++;
#endif

#ifdef JT
	x2 = get_usecs();
	t[38] += (x2 - x1);
	if (x2 - x1 > b[38]) b[38] = x2 - x1;
	if (x2 - x1 < s[38]) s[38] = x2 - x1;
	c[38]++;
#endif
}

#else  /* NULL_LIB */

/**
 * Allocate bytes at multiples of the page size.
 *
 * @param size Number of bytes to be allocated
 */
unsigned long jia_alloc(int size)
{
	/* FIXME: valloc is deprecated */
	return (unsigned long) valloc(size);
}
#endif /* NULL_LIB */
