#include <stdio.h>
#include <stdlib.h>
#include "../jumpsrc/jia.h"

#define KEY    2097152                      /* numbers to be sorted */
#define SEED         1

unsigned int *a;

int main(int argc, char ** argv)
{
   unsigned int *local;
   unsigned int i, count1, count2, count3, temp;
   double time1, time2, time3, time4, time5, t1, t2, x1, x2, x3;   
   unsigned int magic, allocate, start;
   int stage;

   jia_init(argc, argv);
   jia_barrier();

   magic = KEY / jiahosts;
   time1 = jia_clock();

   a = (unsigned int *) 
       jia_alloc(KEY * sizeof(unsigned int));

   allocate = 1;
   t2 = -1.000;

   for (i = 2; ((signed) i) <= jiahosts; i *= 2)
      if (jiapid % i == 0)
         allocate = i;

   local = (unsigned int *) 
           malloc(magic * allocate * sizeof(unsigned int));

   jia_barrier();
   time2 = jia_clock();

   srand(jiapid+SEED);

   temp = 0;

   /* prepare a locally sorted array */

   x1 = jia_clock();
   for (i = 0; i < magic; i++)
   {
      temp = temp + (rand() % 128);
      local[i] = temp;
   }
   x2 = jia_clock();

   printf("Local array init Timing = %2.8f\n", x2-x1);
   start = jiapid * magic;
   stage = 1;

   for (i = 0; i < magic; i++)
      a[start+i] = local[i];
   x3 = jia_clock();
   printf("Shared memory init Timing = %2.8f\n", x3-x2);

   jia_barrier();
   time3 = jia_clock();

   /* the real sorting comes here */

   stage *= 2;

   while (stage <= jiahosts)
   {
      if (jiapid % stage == 0)
      {
         printf("Merging Data in terms of %d procs.\n", stage);

         count1 = jiapid * magic;
         count2 = (jiapid + stage / 2) * magic;
         count3 = 0;

         while (count3 < magic * stage)
         {
            if (count1 == (jiapid + stage / 2) * magic)
               for (i = count2; i < (jiapid + stage) * magic; i++)
               {  
                  local[count3] = a[i];
                  count3++;
               }
            else
               if (count2 == (jiapid + stage) * magic)
                  for (i = count1; i < (jiapid + stage / 2) * magic; i++)
                  {
                     local[count3] = a[i];
                     count3++;
                  }
               else
               {
                  if (a[count1] < a[count2])
                  {  local[count3] = a[count1];
                     count1++;
                     count3++;
                  }
                  else
                  {  local[count3] = a[count2];
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

      t1 = jia_clock();

      if (t2 > 0)
         printf("Substage Timing t1 - t2 = %f\n", t1 - t2);
      else
         printf("Substage Timing t1 - time3 = %f\n", t1 - time3);

      jia_barrier();

      t2 = jia_clock();

      printf("Barrier Timing t2 - t1 = %f\n", t2 - t1);

      stage *= 2;
   }
   
   time4 = jia_clock();

   if (jiapid == 0)
   {
      for (i = 0; i < KEY - 1; i++)
         if (a[i+1] < a[i])
         {  printf("Merge Sort: Error at %d!\n", i);
            break;
         }
   }

   time5 = jia_clock();

   printf("Time\t%f\t%f\t%f\t%f\t%f\t%f\n", time2-time1, time3-time2,
           time4-time3, time5-time4, time5-time5, time5-time1);

   jia_exit();

   return 0;
}
