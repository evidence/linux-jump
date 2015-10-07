#ifndef	JIAGLOBAL_H
#define	JIAGLOBAL_H

#include        <stdio.h>
#include        <stdlib.h>
#include        <memory.h>
#include        <stdarg.h>
#include        <sys/socket.h>
#include        <sys/time.h>
#include        <sys/types.h>
#include        <errno.h>
#include        <netdb.h>
#include        <fcntl.h>
#include        <netinet/in.h>
#include        <arpa/inet.h>
#include        <sys/mman.h>
#include        <sys/stat.h>
#include        <string.h>
#include        <unistd.h>
#include        <pwd.h>
#include 	<signal.h>
#include        <string.h>
#include        <sys/resource.h>
#include        <sys/fcntl.h>
#include        <asm/mman.h> 
#include        "jia.h"

#define sigcontext_struct sigcontext

typedef void (* void_func_handler)();

#ifndef MAX
#define MAX(x, y)  (((x) >= (y)) ? (x) : (y)) 
#endif /* MAX */

#define SPACE(right) { if ((right) == 1) 	\
	printf("\t\t\t"); 	\
}

#ifdef PDEBUG
#define _dprintf(_fmt, ... )                             \
    do {                                                \
    fprintf(stderr, "%s():%d - " _fmt "%s\n",              \
            __func__, __LINE__, __VA_ARGS__);         \
    } while (0);
#define dprintf(...) _dprintf(__VA_ARGS__, "")
#else
#define dprintf(...) do {} while(0);
#endif

/* Using a new data structure (linked-list representation by array) */
/* for faster access of page control structures			    */
typedef struct mem_llist {
	short int head;
	short int tail;
	short int pendprev[Maxmempages];
	short int pendnext[Maxmempages];
} mem_llist_t;

typedef struct cache_llist {
	short int head;
	short int tail;
	short int pendprev[Cachepages];
	short int pendnext[Cachepages];
} cache_llist_t;

#define insert(l, i)					\
{							\
	if (((l.head) == -1) && ((l.tail) == -1)) {	\
		l.pendnext[i] = -1;			\
		l.pendprev[i] = -1;			\
		(l.head) = (l.tail) = i;		\
	} else {					\
		l.pendprev[i] = l.tail;			\
		l.pendnext[l.tail] = i;			\
		l.pendnext[i] = -1;			\
		l.tail = i;				\
	}						\
}

#define dislink(l, i)						\
{								\
	int p, n;						\
								\
	if (((l.head) == -1) && ((l.tail) == -1)){		\
		printf("Dislink: Nothing to dislink\n");	\
	} else {						\
		if (((l.head) == i) && ((l.tail) == i)) {	\
			(l.head) = (l.tail) = -1;		\
		} else if ((l.head) == i) {			\
			l.head = l.pendnext[l.head];		\
			l.pendprev[l.head] = -1;		\
		} else 	if ((l.tail) == i) {			\
			l.tail = l.pendprev[l.tail];		\
			l.pendnext[l.tail] = -1;		\
		} else {					\
			p = l.pendprev[i];			\
			n = l.pendnext[i];			\
			l.pendprev[n] = p;			\
			l.pendnext[p] = n;			\
		}						\
	}							\
}

#endif /* JIAGLOBAL_H */
