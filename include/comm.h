#ifndef	JIACOMM_H
#define	JIACOMM_H

#include "global.h"
#include "init.h"

#define TIMEOUT      1000
#define MAX_RETRIES  64

#define Maxmsgsize   (40960-Msgheadsize) 
#define Msgheadsize  32
#define Maxmsgs      Maxhosts
#define Maxqueue     32

#define  DIFF      0
#define  DIFFGRANT 1     
#define  GETP      2  
#define  GETPGRANT 3     
#define  ACQ       4
#define  ACQGRANT  5    
#define  INVLD     6
#define  BARR      7
#define  BARRGRANT 8
#define  REL       9
#define  WTNT      10 
#define  JIAEXIT   11
#define  WAIT      12
#define  WAITGRANT 13
#define  STAT      18
#define  STATGRANT 19
#define  ERRMSG    20
#define  ACQFWD    21

#define  inqh    inqueue[inhead]
#define  inqt    inqueue[intail]
#define  outqh   outqueue[outhead]
#define  outqt   outqueue[outtail]

typedef struct Jia_Msg {
	int op;
	int frompid;
	int topid;
	int syn;
	int seqno;
	int index;  
	int binterval;
	int size;
	unsigned char data[Maxmsgsize];
} jia_msg_t;

typedef  jia_msg_t* msgp_t;

/**
 * Communication manager
 */
typedef struct CommManager {
	int                 snd_fds[Maxhosts];
	fd_set              snd_set;
	int                 snd_maxfd;
	unsigned            snd_seq[Maxhosts];

	int                 rcv_fds[Maxhosts];
	fd_set              rcv_set;
	int                 rcv_maxfd;
	unsigned            rcv_seq[Maxhosts];
} CommManager;

#endif	/* JIACOMM_H */
