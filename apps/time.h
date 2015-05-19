#include <sys/time.h>

long int time_diff_sec(struct timeval* new, struct timeval* old)
{
	return ((new->tv_sec)-(old->tv_sec)) + ((new->tv_usec)-(old->tv_usec))/1000000;
}

long int time_diff_msec(struct timeval* new, struct timeval* old)
{
	return ((new->tv_sec)-(old->tv_sec))*1000 + ((new->tv_usec)-(old->tv_usec))/1000;
}

long int time_sec(struct timeval* t)
{
	return (t->tv_sec) + ((t->tv_usec)/1000000);
}

long int time_msec(struct timeval* t)
{
	return ((t->tv_sec)*1000) + ((t->tv_usec)/1000);
}

void gettime(struct timeval *t)
{
	gettimeofday(t, NULL);
}

