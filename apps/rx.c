/**
 * This example implements Radix Sort on top of the Jump DSM system.
 */

#include <stdio.h>
#include <stdlib.h>
#include "jia.h"
#include "time.h"
#include "assert.h"

#define MAGIC       16                      /* number of slots */
#define BIT          4                      /* 2 ^ 4 = 16 */
#define KEY    2097152                      /* numbers to be sorted */
#define SEED         1

unsigned int *a;

int main(int argc, char ** argv)
{
	int count[MAGIC];
	unsigned int *local;
	unsigned int value;
	int i, j, k, element, stage, error;
	int remainder, temp1, temp2, temp3, temp;
	struct timeval time1, time2, time3, time4, time5;
	int slot;	/*< Slot size */

	int r_order[2][16], w_order[2][16];
	int o_order[16];

	jia_init(argc, argv);
	printf("Number of hosts: %d\n", jiahosts);
	slot = KEY * 2 / (MAGIC * jiahosts);

	jia_barrier();
	gettime(&time1);

	a = (unsigned int *) 
		jia_alloc(KEY * 2 * sizeof(unsigned int));

	local = (unsigned int *) malloc(KEY * sizeof(unsigned int));

	jia_barrier();
	gettime(&time2);

	j = 0;

	for (i = 0; i < MAGIC; i++) {
		r_order[0][i] = j + jiapid * MAGIC;
		j = j + MAGIC / jiahosts;
		if (j >= MAGIC) j = j - MAGIC + 1;
	}

	j = jiapid * MAGIC / jiahosts;

	for (i = 0; i < MAGIC; i++) {
		r_order[1][i] = j;
		j = j + MAGIC;
		if (j >= MAGIC * jiahosts) j = j - MAGIC * jiahosts + 1;
	}

	j = 0;

	for (i = 0; i < MAGIC; i++) {
		o_order[i] = j;
		j = j + jiahosts;
		if (j >= MAGIC) j = j - MAGIC + 1;
	}

	for (i = 0; i < MAGIC; i++) {
		w_order[0][i] = r_order[0][o_order[i]]; 
		w_order[1][i] = r_order[1][o_order[i]];
	}

	srand(jiapid+SEED);

	for (i = 0; i < KEY / jiahosts; i++) {
		temp1 = rand() % 1024;
		temp2 = rand() % 2048;
		temp3 = rand() % 2048;
		local[i] = (temp1 << 22) + (temp2 << 11) + temp3;
		if(local[i] == 0) 
			local[i] = 1;
	}

	element = KEY / jiahosts;
	stage = sizeof(unsigned int) * 8 / BIT;

	jia_barrier();
	gettime(&time3);

	for (i = 0; i < stage; i++) {
		if (i > 0) {
			printf("Radix sort: swapping stage %d \n", i);
			element = 0;

			for (j = 0; j < MAGIC; j++) {
				temp = r_order[i % 2][j] * slot;
				while (a[temp] > 0) {
					local[element] = a[temp]; 
					element++;
					temp++;
				}
			} 

			jia_barrier();
		}

		if (i < stage) {
			printf("Radix sort: sorting stage %d, %d elements\n", i,
					element);

			for (j = 0; j < MAGIC; j++)
				count[j] = 0;

			for (j = 0; j < element; j++) {
				remainder = (local[j] >> (i * BIT)) % MAGIC;
				if (count[remainder] == (slot - 1)) {
					JIA_ERROR("Number of slots not enough, sorting aborted.\n");
				} else {  
					temp = w_order[i % 2][remainder] * slot + count[remainder];
					a[temp] = local[j];
					count[remainder]++;
				}
			}

			for (j = 0; j < MAGIC; j++)
				a[w_order[i % 2][j] * slot + count[j]] = 0;

			jia_barrier();
		}
	}

	gettime(&time4);

	error = 0;
	value = 0;
	element = 0;

	if (jiapid == 0) {      
		for (k = 0; k < jiahosts; k++) {
			for (j = 0; j < MAGIC; j++) {
				temp = (r_order[stage % 2][j] + k * MAGIC) * slot;
				while (a[temp] > 0) {
					if (value > a[temp])
						error = 1;
					value = a[temp];
					element++;
					temp++;
				}
			}
		} 

		if (error == 1)
			printf("Radix sort: Sorting error!\n");
		if (element != KEY) {
			printf("Radix sort: Error number of keys!\n");
			error = 1;
		}
		if (error == 0)
			printf("Radix sort: Success!\n");

	}

	gettime(&time5);

	printf("Partial time 1:\t\t %ld msec\n", time_diff_msec(&time2, &time1));
	printf("Partial time 2:\t\t %ld msec\n", time_diff_msec(&time3, &time2));
	printf("Partial time 3:\t\t %ld msec\n", time_diff_msec(&time4, &time3));
	printf("Partial time 4:\t\t %ld msec\n", time_diff_msec(&time5, &time4));
	printf("Total time:\t\t %ld msec\n", time_diff_msec(&time5, &time1));

	jia_exit();

	return 0;
}
