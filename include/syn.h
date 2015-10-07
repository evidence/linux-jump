#ifndef JIASYN_H
#define JIASYN_H

#define Maxwtnts     511
#define Maxstacksize 5
#define hidelock     (Maxlocks + Maxcondvs)
#define top          lockstack[stackptr]
#define stol(x)      (*((unsigned long *)(x)))
#define stoi(x)      (*((unsigned int *)(x)))
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
