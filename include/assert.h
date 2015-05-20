#ifndef	ASSERT_H
#define	ASSERT_H

#include "comm.h"

extern void asendmsg(jia_msg_t *msg);

#define ASSERT(cond, ...)									\
{												\
	if (!cond) {										\
		fprintf(stderr, "ASSERT ERROR Host %d: __FILE__: __LINE__\n\t", jiapid);	\
		fprintf(stderr, __VA_ARGS__);							\
		fflush(stderr);									\
		fflush(stdout);									\
		exit(-1);									\
	}											\
}

#define RASSERT(cond, ...)							\
{										\
	if (!cond) {								\
		jia_msg_t assertmsg;						\
		int hosti;							\
		assertmsg.op = JIAEXIT;						\
		assertmsg.frompid = jia_pid;					\
		snprintf((char*) assertmsg.data, Maxmsgsize, __VA_ARGS__);	\
		for (hosti = 0; hosti < hostc; hosti++)				\
			if (hosti != jia_pid) {					\
				assertmsg.topid = hosti;			\
				asendmsg(&assertmsg);				\
			}							\
		assertmsg.topid = jia_pid;					\
		asendmsg(&assertmsg);						\
	}									\
}


#ifndef NULL_LIB

#define JIA_ERROR(...) RASSERT (0, __VA_ARGS__)

#else

#define JIA_ERROR(...)					\
{							\
	printf("JUMP error --- %s\n", __VA_ARGS__);	\
	exit(-1);					\
}


#endif /* NULL_LIB */
#endif /* ASSERT_H */
