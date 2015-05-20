#ifndef JIA_PUBLIC
#define	JIA_PUBLIC

/**
 * Maximum number of hosts.
 *
 * The number of hosts found in ~/.jiahosts file cannot exceed this value.
 */
#define Maxhosts   16
#define	Maxlocks   1024
#define	Maxcondvs  64 
#define Intbytes   4
#define Intbits    32
#define Shortbytes 2
#define Pagesize   4096

#ifndef Cachepages
#define Cachepages 1024
#endif

#define Startaddr   0x60000000
#define Maxmemsize  0x08000000  
#define Maxmempages (Maxmemsize / Pagesize)

#define jiahosts  hostc
#define jiapid    jia_pid

void           jia_init(int, char **);
unsigned long  jia_alloc3(int,int,int);
unsigned long  jia_alloc2(int,int);
unsigned long  jia_alloc(int);
void           jia_lock(int);
void           jia_unlock(int);
void           jia_barrier();
void           jia_wait();
void           jia_exit();
float          jia_clock();

extern int	jia_pid;
extern int      hostc;
extern int      jia_lock_index;

#define	LOCKDEC(x)	int x;
#ifndef NULL_LIB
#define	LOCKINIT(x)     (x) = (jia_lock_index++) % Maxlocks;
#else /* NULL_LIB */
#define	LOCKINIT(x)     
#endif /* NULL_LIB */
#define	QLOCK(x)	jia_lock(x);
#define	QUNLOCK(x)	jia_unlock(x);

#define	ALOCKDEC(x,y)	int x[y];
#define	ALOCKINIT(x,y)	{						\
	int i;		 				\
	for (i = 0; i < (y); i++) {			\
		(x)[i] = (jia_lock_index++) % Maxlocks;	\
	}						\
}
#define	ALOCK(x,y)	jia_lock((x)[y]);
#define	AULOCK(x,y)	jia_unlock((x)[y]);

#define	BARDEC(x)	int x;
#define	BARINIT(x)
#define	BARRIER(x,y)	jia_barrier();

#define	EXTERN_ENV		
#define	MAIN_ENV		
#define	MAIN_INITENV(x,y)			
#define	MAIN_END 
#define	WAIT_FOR_END(x)	jia_barrier();

#define	PAUSEDEC(x)	int x;
#define	PAUSEINIT(x)	
#define	PAUSE_POLL(x)
#define	PAUSE_SET(x,y)	
#define	PAUSE_RESET(x)	
#define	PAUSE_ACCEPT(x)	

#define	GSDEC(x)	int x;
#define	GSINIT(x)	
#define	GETSUB(x, y, z, zz)	

#define	G_MALLOC(x)	jia_alloc(x);
#define	G2_MALLOC(x,y)	jia_alloc(x);

#endif /* JIA_PUBLIC */
