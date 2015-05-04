#include <stdio.h>
#include <stdlib.h>
#include "../jumpsrc/jia.h"

#define MAGIC       16                      /* number of slots */
#define BIT          4                      /* 2 ^ 4 = 16 */
#define KEY    2097152                      /* numbers to be sorted */
#define PROCS        4
#define SLOT   KEY * 2 / (MAGIC * PROCS)    /* slot size */
#define SEED         1

unsigned int *a;

int main(int argc, char ** argv)
{
	int count[MAGIC];
	unsigned int *local;
	unsigned int value;
	int i, j, k, element, stage, error;
	int remainder, temp1, temp2, temp3, temp;
	double time1, time2, time3, time4, time5;

	int r_order[2][16], w_order[2][16];
	int o_order[16];

	jia_init(argc, argv);

	jia_barrier();
	time1 = jia_clock();

	a = (unsigned int *) 
		jia_alloc(PROCS * MAGIC * SLOT * sizeof(unsigned int));

	local = (unsigned int *) malloc(KEY * sizeof(unsigned int));

	jia_barrier();
	time2 = jia_clock();

	j = 0;

	for (i = 0; i < MAGIC; i++)
	{
		r_order[0][i] = j + jiapid * MAGIC;
		j = j + MAGIC / PROCS;
		if (j >= MAGIC) j = j - MAGIC + 1;
	}

	j = jiapid * MAGIC / PROCS;

	for (i = 0; i < MAGIC; i++)
	{
		r_order[1][i] = j;
		j = j + MAGIC;
		if (j >= MAGIC * PROCS) j = j - MAGIC * PROCS + 1;
	}

	j = 0;

	for (i = 0; i < MAGIC; i++)
	{
		o_order[i] = j;
		j = j + PROCS;
		if (j >= MAGIC) j = j - MAGIC + 1;
	}

	for (i = 0; i < MAGIC; i++)
	{
		w_order[0][i] = r_order[0][o_order[i]]; 
		w_order[1][i] = r_order[1][o_order[i]];
	}

	srand(jiapid+SEED);

	for (i = 0; i < KEY / jiahosts; i++)
	{
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
	time3 = jia_clock();

	for (i = 0; i < stage; i++)
	{
		if (i > 0)
		{
			printf("Radix sort: swapping stage %d \n", i);
			element = 0;

			for (j = 0; j < MAGIC; j++)
			{
				temp = r_order[i % 2][j] * SLOT;
				while (a[temp] > 0)
				{
					local[element] = a[temp]; 
					element++;
					temp++;
				}
			} 

			jia_barrier();
		}

		if (i < stage)
		{
			printf("Radix sort: sorting stage %d, %d elements\n", i,
					element);

			for (j = 0; j < MAGIC; j++)
				count[j] = 0;

			for (j = 0; j < element; j++)
			{
				remainder = (local[j] >> (i * BIT)) % MAGIC;
				if (count[remainder] == SLOT - 1)
					jia_error("Number of slots not enough, sorting aborted.\n");
				else
				{  
					temp = w_order[i % 2][remainder] * SLOT + count[remainder];
					a[temp] = local[j];
					count[remainder]++;
				}
			}

			for (j = 0; j < MAGIC; j++)
				a[w_order[i % 2][j] * SLOT + count[j]] = 0;

			jia_barrier();
		}
	}

	time4 = jia_clock();

	error = 0;
	value = 0;
	element = 0;

	if (jiapid == 0)
	{      
		for (k = 0; k < jiahosts; k++)
		{
			for (j = 0; j < MAGIC; j++)
			{
				temp = (r_order[stage % 2][j] + k * MAGIC) * SLOT;
				while (a[temp] > 0)
				{
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
		if (element != KEY)
		{
			printf("Radix sort: Error number of keys!\n");
			error = 1;
		}
		if (error == 0)
			printf("Radix sort: Success!\n");

	}

	time5 = jia_clock();

	printf("Time\t%f\t%f\t%f\t%f\t%f\t%f\n", time2-time1, time3-time2,
			time4-time3, time5-time4, time5-time5, time5-time1);

	jia_exit();

	return 0;
}
