#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include "jia.h"

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

		if (CHECK==1){
			old=(double **)malloc(N*sizeof(double *));
			for (i=0;i<N;i++){
				old[i]=(double *)malloc(N*sizeof(double));
			}
			new=(double **)malloc(N*sizeof(long));
			for (i=0;i<N;i++){
				new[i]=(double *)malloc(N*sizeof(double));
			}
		}

		srand48(flag);

		for (i=0; i<N; i++){
			for (j=0; j<N; j++) {
				a[j][i] = ((double) lrand48())/MAXRAND;
				if (i==j) a[j][i] *= 10.0;
				if (CHECK==1) old[j][i] = a[j][i];
			}
		}
	}
}

void lua()
{register int i,j,k;
	int begin;
	double temp;

	printf("Begin LU factorization\n"); 

	for (j=0;j<N;j++){
		/*   printf("Step %d:\n", j); */
		if ((j%jiahosts)==jiapid){
			/*
			   printf("%d\t%d\t%d\t%lf\textraz\n", j, j, j, a[j][j]);
			   */
			if (fabs(a[j][j])>EPSILON){
				temp=a[j][j];
				/*
				   if (j <= 8)
				   printf("%d:a[%d][%d]:%lf:z\n",j,j,j,temp);
				   */
				for (i=j+1;(i<N);i++){
					a[j][i]/=temp;
					/*
					   if (i <= 8 && j <= 8)
					   printf("%d:a[%d][%d]:%lf:z\n",j,j,i,a[j][i]);
					   */
				}
			}else{
				sprintf(luerr,"Matrix a is singular, a[%d][%d]=%f",j,j,a[j][j]);
				jia_error(luerr); 
			}
		}

		/*   printf("+B "); */
		jia_barrier();
		/*   printf("-B "); */

		begin=j+1-(j+1)%jiahosts+jiapid;
		if (begin<(j+1)) begin+=jiahosts;
		for (k=begin;k<N;k+=jiahosts){
			temp=a[k][j];
			/*
			   printf("%d\t%d\t%d\t%lf\textra_x\n", j, k, j, temp); 
			   */
			for (i=j+1;(i<N);i++){
				/*
				   if (i > 123) printf("%d\t%d\t%d\t%lf\textra_y\n", j, k, i,
				   a[k][i]);
				   if (a[j][i] > 100 || a[j][i] < -100)
				   printf("%d\t%d\t%d\t%lf\textra_x\n", j, j, i, a[j][i]); 
				   */
				a[k][i]-=(a[j][i]*temp);
				/*
				   if (i > 123) printf("%d\t%d\t%d\t%lf\textra_y\n", j, k, i, a[k][i]);
				   */
			}
		}
	}
}


void checka()
{int i,j,k;
	double temp;
	int correct = 1;

	if ((CHECK==1)&&(jiapid==0)){
		printf("Begin check the result!\n");

		for (i=0;i<N;i++)
			for (j=0;j<N;j++){
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
				/*
				   else if (j==0){ 
				   printf("old[%d][%d]=%14.6lf, new[%d][%d]=%14.6lf\n",
				   i,j,old[j][i],i,j,new[j][i]);
				   }
				   */
			}

		if (correct)
			printf("Happily pass check! Correct LU factorization!\n");
	}
}

int main(int argc, char **argv)
{
	float tt1, tt2, tt3, tt4, tt5;

	printf("Seed is %lu\n", flag);

	jia_init(argc,argv);

	jia_barrier();
	tt1 = jia_clock();

	a=(double (*)[N])jia_alloc3(N*N*sizeof(double),(N*N*sizeof(double))/jiahosts,0);

	jia_barrier();
	tt2 = jia_clock();

	seqinita(flag);
	jia_barrier();
	tt3 = jia_clock();
	lua();
	jia_barrier();
	tt4 = jia_clock();

	checka();
	jia_barrier(); 
	tt5 = jia_clock();

	printf("Time\t%f\t%f\t%f\t%f\t%f\t%f\n", 
			tt2-tt1, tt3-tt2, tt4-tt3, tt5-tt4, tt5-tt5, tt5-tt1);

	jia_exit();

	return 0;
}
