#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#define N 100000000

static double timer(void)
{
  struct timeval tm;
  gettimeofday (&tm, NULL);
  return tm.tv_sec + tm.tv_usec/1000000.0;
}

int main (int argc, char **argv)
{
	double clock;
	int i;
	unsigned long long sum;
	clock = timer(); /* start timer */
	sum = 0;
	#pragma omp parallel reduction(+:sum)
	{
		#pragma omp for
		for (i=0; i<N; ++i)
		{
			sum = sum + i;
		}
	}
	clock = timer() - clock; /* stop timer */
	printf ("sum=%llu\n", sum);
	printf ("wallclock: %lf seconds\n", clock);
	printf("*************************************\n");
	clock = timer();   /* start timer */
	sum=0;
	#pragma omp parallel shared(sum)
	{
		#pragma omp for
		for (i=0; i<N; ++i)
		{
			#pragma omp critical
			{
				sum = sum + i;
			}
		}
	}
	clock = timer() - clock;    /* stop timer */
	printf ("sum=%llu\n", sum);
	printf ("wallclock: %lf seconds\n", clock);
	
	printf("\n");
	system("echo $USER");
	system("date");
	return EXIT_SUCCESS;
}

