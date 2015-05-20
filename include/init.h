#ifndef JIACREAT_H
#define JIACREAT_H

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
/**
 * Data structure used for statistical information.
 */
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
