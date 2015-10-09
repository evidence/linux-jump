/**
 * This example implements Merge Sort on top of the Jump DSM system.
 */

#include <stdio.h>
#include <stdlib.h>
#include "jia.h"
#include "time.h"

#define KEY    2097152                  /* numbers to be sorted */
#define SEED         1

unsigned int *a;

int main(int argc, char ** argv)
{
	unsigned int *local;
	unsigned int i, count1, count2, count3, temp;
	struct timeval t1, t2, x1, x2, x3;   
	struct timeval time1, time2, time3, time4, time5;
	unsigned int magic, allocate, start;
	int stage;

	jia_init(argc, argv);
	jia_barrier();

	magic = KEY / jiahosts;
	gettime(&time1);

	a = (unsigned int *) 
		jia_alloc(KEY * sizeof(unsigned int));

	allocate = 1;

	for (i = 2; ((signed) i) <= jiahosts; i *= 2)
		if (jiapid % i == 0)
			allocate = i;

	local = (unsigned int *) 
		malloc(magic * allocate * sizeof(unsigned int));

	jia_barrier();
	gettime(&time2);

	srand(jiapid+SEED);

	temp = 0;

	/* prepare a locally sorted array */
	gettime(&x1);
	for (i = 0; i < magic; i++) {
		temp = temp + (rand() % 128);
		local[i] = temp;
	}
	gettime(&x2);

	printf("Local array init Timing = %ld msec\n", time_diff_msec(&x2, &x1));
	start = jiapid * magic;
	stage = 1;

	for (i = 0; i < magic; i++)
		a[start+i] = local[i];
	gettime(&x3);
	printf("Shared memory init Timing = %ld msec\n", time_diff_msec(&x3, &x2));

	jia_barrier();
	gettime(&time3);

	/* the real sorting comes here */

	stage *= 2;

	while (stage <= jiahosts) {
		if (jiapid % stage == 0) {
			printf("Merging Data in terms of %d procs.\n", stage);

			count1 = jiapid * magic;
			count2 = (jiapid + stage / 2) * magic;
			count3 = 0;

			while (count3 < magic * stage) {
				if (count1 == (jiapid + stage / 2) * magic) {
					for (i = count2; i < (jiapid + stage) * magic; i++){  
						local[count3] = a[i];
						count3++;
					}
				} else if (count2 == (jiapid + stage) * magic) {
					for (i = count1; i < (jiapid + stage / 2) * magic; i++){
						local[count3] = a[i];
						count3++;
					}
				} else {
						if (a[count1] < a[count2]) {  
							local[count3] = a[count1];
							count1++;
							count3++;
						} else {  
							local[count3] = a[count2];
							count2++;
							count3++;
						}
				}
			}

			start = jiapid * magic;
			for (i = 0; i < stage * magic; i++)
				a[start+i] = local[i];

			printf("Merge done, count = %d (%d).\n", count3, magic * stage);
		}

		gettime(&t1);
		jia_barrier();
		gettime(&t2);

		printf("Barrier Timing t2 - t1 = %ld msec\n", time_diff_msec(&t2, &t1));

		stage *= 2;
	}

	gettime(&time4);

	if (jiapid == 0) {
		for (i = 0; i < KEY - 1; i++)
			if (a[i+1] < a[i]) {  
				printf("Merge Sort: Error at %d!\n", i);
				break;
			}
	}

	gettime(&time5);

	printf("Partial time 1:\t\t %ld msec\n", time_diff_msec(&time2, &time1));
	printf("Partial time 2 (parallel):\t %ld msec\n", time_diff_msec(&time3, &time2));
	printf("Partial time 3:\t\t %ld msec\n", time_diff_msec(&time4, &time3));
	printf("Partial time 4:\t\t %ld msec\n", time_diff_msec(&time5, &time4));
	printf("Total time:\t\t %ld msec\n", time_diff_msec(&time5, &time1));


	jia_exit();

	return 0;
}
