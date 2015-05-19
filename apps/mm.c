/**
 * This example implements Matrix Multiplication on top of the Jump DSM system.
 */

#include <stdio.h>
#include "jia.h"
#include "time.h"

extern unsigned int jia_startstat();

#define N 512
int (*a)[N], (*b)[N], (*c)[N];

void seqinit()
{
	int i,j;

	if (jiapid == 0) 
		for (i = 0; i < N; i++) 
			for (j = 0; j < N; j++) {
				a[i][j] = 1;
				b[i][j] = 1;
			}
}

int main(int argc, char **argv)
{
	int x, y, z, p, magic;
	struct timeval time0, time1, time2, time3, time4, time5;
	int error;
	int local[N][N] = { {0} };

	jia_init(argc, argv);

	jia_barrier();
	gettime(&time0);

	a = (int (*)[N])jia_alloc(N * N * sizeof(int));
	b = (int (*)[N])jia_alloc(N * N * sizeof(int));
	c = (int (*)[N])jia_alloc(N * N * sizeof(int));

	printf("Memory Allocation Done\n");

	if (N * N * 4 < 4096 * jiahosts)
		printf("NOTE: There is false sharing in this program execution!\n");

	jia_barrier();
	gettime(&time1);

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
	gettime(&time2);

	printf("Calculating local ....................\n");
	for (x = jiapid * magic; x < (jiapid + 1) * magic; x++)
		for (y = 0; y < N; y++)
			for (z = 0; z < N; z++)
				local[y][z] += (a[x][y] * b[x][z]);

	printf("Calculating local matrix done!\n");
	jia_barrier();

	gettime(&time3);

	for (x = 0; x < jiahosts; x++) { 
		p = (jiapid + x) % jiahosts;
		jia_lock(p);
		for (y = p * magic; y < (p + 1) * magic; y++) {  
			for (z = 0; z < N; z++) {
				if (x == 0)
					c[y][z] = local[y][z];
				else
					c[y][z] += local[y][z];
			}
		}
		jia_unlock(p);
		if (x == 0) jia_barrier();
	}

	jia_barrier();

	gettime(&time4);

	if (jiapid == 0) {
		error = 0;
		for (x = 0; x < jiahosts; x++) {
			jia_lock(x);
			for (y = x * magic; y < (x + 1) * magic; y++) {
				for (z = 0; z < N; z++) {
					if (c[y][z] != N) {  
						error = 1;
						printf("ERROR: y = %d, z = %d, c = %d\n", y, z, c[y][z]);
					}
				}
			}
			jia_unlock(x);
		}
		if (error == 1) 
			printf("NOTE: There is some error in the program!\n");
	}

	gettime(&time5);

	printf("Partial time 0:\t\t %ld sec\n", time_diff_sec(&time1, &time0));
	printf("Partial time 1:\t\t %ld sec\n", time_diff_sec(&time2, &time1));
	printf("Partial time 2:\t\t %ld sec\n", time_diff_sec(&time3, &time2));
	printf("Partial time 3:\t\t %ld sec\n", time_diff_sec(&time4, &time3));
	printf("Partial time 4:\t\t %ld sec\n", time_diff_sec(&time5, &time4));
	printf("Total time:\t\t %ld sec\n", time_diff_sec(&time5, &time1));


	jia_exit();
	return 0;
} 

