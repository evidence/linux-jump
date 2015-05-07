#include <stdio.h>
#include "jia.h"

extern unsigned int jia_startstat();

#define N 512
int (*a)[N], (*b)[N], (*c)[N];

void seqinit()
{
	int i,j;

	if (jiapid == 0) 
	{
		for (i = 0; i < N; i++) 
		{  /* printf("i = %d\n", i); */
			for (j = 0; j < N; j++)
			{
				a[i][j] = 1;
				b[i][j] = 1;
			}
		}
	}
}

int main(int argc, char **argv)
{
	int x, y, z, p, magic;
	float t0, t1, t2, t3, t4, t5;
	int error;
	int local[N][N] = { {0} };

	jia_init(argc, argv);

	jia_barrier();
	t0 = jia_clock();

	a = (int (*)[N])jia_alloc(N * N * sizeof(int));
	b = (int (*)[N])jia_alloc(N * N * sizeof(int));
	c = (int (*)[N])jia_alloc(N * N * sizeof(int));

	printf("Memory Allocation Done\n");

	if (N * N * 4 < 4096 * jiahosts)
		printf("NOTE: There is false sharing in this program execution!\n");

	jia_barrier();
	t1 = jia_clock();

	printf("STAT: starting barrier\n");
	jia_barrier();
	printf("STAT: exiting barrier\n"); 

	printf("Initializing matrices ....................\n");
	seqinit();
	printf("Initializing matrices done!\n");

	printf("STAT: starting barrier 2\n");
	jia_barrier();
	printf("STAT: exiting barrier 2\n");

	magic = N / jiahosts;

	jia_startstat();
	t2 = jia_clock();

	/* printf("Calculating local ....................\n"); */
	for (x = jiapid * magic; x < (jiapid + 1) * magic; x++)
		for (y = 0; y < N; y++)
			for (z = 0; z < N; z++)
				local[y][z] += (a[x][y] * b[x][z]);

	/* printf("Calculating local matrix done!\n"); */
	/* printf("STAT: starting barrier\n"); */
	jia_barrier();
	/* printf("STAT: exiting barrier\n"); */

	t3 = jia_clock();

	for (x = 0; x < jiahosts; x++) 
	{
		p = (jiapid + x) % jiahosts;
		/* printf("STAT: getting lock %d\n ", p); */
		jia_lock(p);
		/* printf("STAT: Acquire done.\n"); */
		for (y = p * magic; y < (p + 1) * magic; y++)
		{  
			for (z = 0; z < N; z++)
			{  if (x == 0) c[y][z] = local[y][z];
				else c[y][z] += local[y][z];
			}
			/* printf("\n"); */
		}
		/* printf("STAT: Releasing lock %d\n", p); */
		jia_unlock(p);
		/* printf("STAT: Release Done.\n"); */
		if (x == 0) jia_barrier();
	}

	/* printf("STAT: starting barrier\n"); */
	jia_barrier();
	/* printf("STAT: exiting barrier\n"); */

	t4 = jia_clock();

	if (jiapid == 0)
	{
		error = 0;
		for (x = 0; x < jiahosts; x++)
		{
			/* printf("STAT: Getting lock %d\n", x); */
			jia_lock(x);
			/* printf("STAT: Acquire done.\n"); */
			for (y = x * magic; y < (x + 1) * magic; y++)
			{
				for (z = 0; z < N; z++)
				{
					if (c[y][z] != N) 
					{  error = 1;
						printf("ERROR: y = %d, z = %d, c = %d\n", y, z, c[y][z]);
					}
				}
			}
			/* printf("STAT: Releasing lock %d\n", x); */
			jia_unlock(x);
			/* printf("STAT: Release Done.\n"); */
		}
		if (error == 1) 
			printf("NOTE: There is some error in the program!\n");
	}

	t5 = jia_clock();
	printf("Time\t%f\t%f\t%f\t%f\t%f\t%f\n", t1-t0, t2-t1, t3-t2, t4-t3,
			t5-t4, t5-t0);

	jia_exit();
	return 0;
} 

