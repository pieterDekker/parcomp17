/* file: memory.c */
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
int main (int argc, char **argv)
{
	printf("num threads: %d\n", omp_get_num_threads());
	
	int i=31415;

	printf ("#i=%d\n", i);
	#pragma omp parallel private(i)
	{
		printf (" i=%d\n", i);//all 0
		i = 99;
	}

	printf ("#i=%d\n", i);
	#pragma omp parallel firstprivate(i)
	{
		printf (" i=%d\n", i);//all 31415
		i = 99;
	}

	printf ("#i=%d\n", i);
	#pragma omp parallel shared(i)
	{
		printf (" i=%d\n", i++);//31415-31418
	}
	printf ("#i=%d\n", i);
	
	printf("\n");
	system("echo $USER");
	system("date");
	return EXIT_SUCCESS;
}

