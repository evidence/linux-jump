/**
 * This example implements LU Factorization on top of the Jump DSM system.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include "jia.h"
#include "time.h"

#define N         512
#define CHECK     1
#define EPSILON   1e-5
#define MAXRAND   32767.0
#define min(a,b)  (((a)<(b)) ? (a) : (b))
#define fabs(a)   (((a)>0.0) ? (a) : (0.0-(a)))

double (*a)[N];                 
double **old,**new;
char luerr[80];
unsigned long int flag=1;

void seqinita()
{
	int i,j;

	if (jiapid==0) {
		printf("Begin initialization\n"); 

		if (CHECK==1) {
			old=(double **)malloc(N*sizeof(double *));
			for (i=0;i<N;i++)
				old[i]=(double *)malloc(N*sizeof(double));
			new=(double **)malloc(N*sizeof(long));
			for (i=0;i<N;i++)
				new[i]=(double *)malloc(N*sizeof(double));
		}

		srand48(flag);

		for (i=0; i<N; i++) {
			for (j=0; j<N; j++) {
				if (i==j)
					a[j][i] *= 10.0;
				else
					a[j][i] = ((double) lrand48())/MAXRAND;
				if (CHECK == 1) 
					old[j][i] = a[j][i];
			}
		}
	}
}

void lua()
{
	register int i,j,k;
	int begin;
	double temp;

	printf("Begin LU factorization\n"); 

	for (j=0; j<N; j++){
		if ((j%jiahosts) == jiapid) {
			if (fabs(a[j][j])>EPSILON) {
				temp=a[j][j];
				for (i=j+1;(i<N);i++)
					a[j][i]/=temp;
			} else {
				sprintf(luerr,"Matrix a is singular, a[%d][%d]=%f",j,j,a[j][j]);
				jia_error(luerr); 
			}
		}

		jia_barrier();

		begin = j+1-(j+1)%jiahosts + jiapid;
		if (begin < (j+1))
			begin += jiahosts;
		for (k=begin; k<N; k+=jiahosts){
			temp = a[k][j];
			for (i=j+1; (i<N); i++)
				a[k][i] -= (a[j][i]*temp);
		}
	}
}


void checka()
{
	int i,j,k;
	double temp;
	int correct = 1;

	if ((CHECK==1) && (jiapid==0)){
		printf("Begin check the result!\n");

		for (i=0;i<N;i++)
			for (j=0;j<N;j++) {
				temp=0.0;

				for (k=0;k<=min(i,j);k++)
					if (i==k) temp+=a[j][k];
					else      temp+=a[k][i]*a[j][k];

				new[j][i]=temp;

				if (fabs(old[j][i]-new[j][i])>EPSILON){
					printf("Incorrect! old[%d][%d]=%f, new[%d][%d]=%f\n",
							i,j,old[j][i],i,j,new[j][i]);
					correct = 0;
				}
			}

		if (correct)
			printf("Happily pass check! Correct LU factorization!\n");
	}
}

int main(int argc, char **argv)
{
	struct timeval time1, time2, time3, time4, time5;

	printf("Seed is %lu\n", flag);

	jia_init(argc,argv);

	jia_barrier();
	gettime(&time1);

	a=(double (*)[N])jia_alloc3(N*N*sizeof(double),(N*N*sizeof(double))/jiahosts,0);

	jia_barrier();
	gettime(&time2);

	seqinita(flag);
	jia_barrier();
	gettime(&time3);
	lua();
	jia_barrier();
	gettime(&time4);

	checka();
	jia_barrier(); 
	gettime(&time5);

	printf("Partial time 1:\t\t %ld sec\n", time_diff_sec(&time2, &time1));
	printf("Partial time 2:\t\t %ld sec\n", time_diff_sec(&time3, &time2));
	printf("Partial time 3:\t\t %ld sec\n", time_diff_sec(&time4, &time3));
	printf("Partial time 4:\t\t %ld sec\n", time_diff_sec(&time5, &time4));
	printf("Total time:\t\t %ld sec\n", time_diff_sec(&time5, &time1));

	jia_exit();

	return 0;
}
