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

#endif /* JIAGLOBAL_H */
